#pragma once

#include <sstream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <termkey.h>

namespace bio = boost::asio;

class Input
{
public:
    Input(bio::io_context &io_context)
        : _descriptor{io_context}
        , _timer{io_context}
    {
        _tk = ::termkey_new(0, 0);
        if (!_tk)
            throw std::runtime_error("Cannot allocate termkey instance");

        _descriptor.assign(STDIN_FILENO);
    }

    ~Input()
    {
        ::termkey_destroy(_tk);
    }

    void Start()
    {
        _ReadInput();
    }

private:
    bio::posix::stream_descriptor _descriptor;
    bio::deadline_timer _timer;
    TermKey *_tk;
    int _nextwait = -1;

    void _OnKey(TermKeyKey &key)
    {
        char buffer[50];
        ::termkey_strfkey(_tk, buffer, sizeof(buffer), &key, TERMKEY_FORMAT_VIM);
        printf("%s\n", buffer);
    }

    void _ReadInput()
    {
        if (_nextwait == -1)
            _timer.cancel();
        else
        {
            _timer.expires_from_now(boost::posix_time::millisec(_nextwait));
            _timer.async_wait([this](const auto &err) {
                TermKeyKey key;
                if (::termkey_getkey_force(_tk, &key) == TERMKEY_RES_KEY)
                    _OnKey(key);
            });
        }

        _descriptor.async_read_some(bio::null_buffers{}, [this](const auto &err, size_t) {
            if (err)
            {
                std::ostringstream oss;
                oss << "Error: " << err;
                throw std::runtime_error(oss.str());
            }

            ::termkey_advisereadable(_tk);

            TermKeyResult ret;
            TermKeyKey key;

            while ((ret = termkey_getkey(_tk, &key)) == TERMKEY_RES_KEY)
                _OnKey(key);

            _nextwait = (ret == TERMKEY_RES_AGAIN) ? termkey_get_waittime(_tk) : -1;
            _ReadInput();
        });
    }
};

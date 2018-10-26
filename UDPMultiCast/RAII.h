#pragma once
#include "UDPMultiCast/includes.h"


namespace RAII
{
    struct socket
    {
        socket() :
            _socket(INVALID_SOCKET)
        {}
        explicit socket(SOCKET s_) :
            _socket(s_)
        {}

        ~socket()
        {
            cleanUp();
        }

        SOCKET const & get() const
        {
            return _socket;
        }

        void set(SOCKET s_)
        {
            _socket = s_;
        }

        void cleanUp();

    private:
        SOCKET _socket;
    };

    struct handle
    {
        handle() :
            _handle(INVALID_HANDLE_VALUE)
        {}
        explicit handle(HANDLE h_) :
            _handle(h_)
        {}

        ~handle()
        {
            cleanUp();
        }

        HANDLE const & get() const
        {
            return _handle;
        }

        void set(HANDLE h_)
        {
            _handle = h_;
        }

        void cleanUp();

    private:
        HANDLE _handle;
    };
}
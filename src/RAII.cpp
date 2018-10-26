#include "UDPMultiCast/RAII.h"
#include "UDPMultiCast/utils.h"


namespace RAII
{
    void socket::cleanUp()
    {
        if (_socket == INVALID_SOCKET)
            return;

        SOCKET temp = _socket;
        _socket = INVALID_SOCKET;

        if (SOCKET_ERROR == ::shutdown(temp, 2))
        {
            ErrorExit("shutdown");
        }
        if (SOCKET_ERROR == ::closesocket(temp))
        {
            ErrorExit("closesocket");
        }
    }

    void handle::cleanUp()
    {
        if (_handle == INVALID_HANDLE_VALUE)
            return;

        HANDLE temp = _handle;
        _handle = INVALID_HANDLE_VALUE;

        if (0 == ::CloseHandle(temp))
        {
            ErrorExit("CloseHandle");
        }
    }
}
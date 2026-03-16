#include "error/dberror.h"

DbError::DbError(ErrorCode code,
            std::string message,
            size_t position)
        : _code(code),
          _position(position) {
    _message = message + ", pos: " + std::to_string(_position);
}

const char* DbError::what() const noexcept {
    return _message.c_str();
}

ErrorCode DbError::code() const { return _code; }
size_t DbError::position() const { return _position; }


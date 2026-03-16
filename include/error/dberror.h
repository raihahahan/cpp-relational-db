#include <exception>
#include <string>

enum class ErrorCode {
    SyntaxError,
    ParseError,
    UndefinedTable,
    UndefinedColumn,
    TypeMismatch,
};

class DbError : public std::exception {
public:
    explicit DbError(ErrorCode code,
            std::string message,
            size_t position = 0);

    const char* what() const noexcept override;

    ErrorCode code() const;
    size_t position() const;

private:
    ErrorCode _code;
    std::string _message;
    size_t _position;
};

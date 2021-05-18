//
// Created by l2pic on 17.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_EXCEPTION_H_
#define CHATCONTROLLER_COMMON_EXCEPTION_H_

#include <stdexcept>

/// This is the base class for all exceptions
class Exception : public std::exception
{
  public:
    /// Creates an exception.
    explicit Exception(const std::string &msg, int code = 0);

    /// Creates an exception.
    Exception(const std::string &msg, const std::string &arg, int code = 0);

    /// Copy constructor.
    Exception(const Exception &exc);

    /// Destroys the exception.
    ~Exception() override;

    /// Assignment operator.
    Exception &operator=(const Exception &exc);

    /// Returns a static string describing the exception.
    [[nodiscard]] virtual const char *name() const noexcept;

    /// Returns the name of the exception class.
    [[nodiscard]] virtual const char *className() const noexcept;

    /// Returns a static string describing the exception.
    ///
    /// Same as name(), but for compatibility with std::exception.
    [[nodiscard]] virtual const char *what() const noexcept;

    /// Returns the message text.
    [[nodiscard]] const std::string &message() const {
        return _msg;
    }

    /// Returns the exception code if defined.
    [[nodiscard]] int code() const {
        return _code;
    }

    /// Returns a string consisting of the
    /// message name and the message text.
    [[nodiscard]] std::string displayText() const;

    /// Creates an exact copy of the exception.
    ///
    /// The copy can later be thrown again by
    /// invoking rethrow() on it.
    [[nodiscard]] virtual Exception *clone() const;

    /// (Re)Throws the exception.
    ///
    /// This is useful for temporarily storing a
    /// copy of an exception (see clone()), then
    /// throwing it again.
    virtual void rethrow() const;

  protected:
    /// Standard constructor.
    explicit Exception(int code = 0);

    /// Sets the message for the exception.
    void message(const std::string &msg) {
        _msg = msg;
    }

    /// Sets the extended message for the exception.
    void extendedMessage(const std::string &arg);
  private:
    std::string _msg;
    int _code;
};

//
// Macros for quickly declaring and implementing exception classes.
// Unfortunately, we cannot use a template here because character
// pointers (which we need for specifying the exception name)
// are not allowed as template arguments.
//
#define DECLARE_EXCEPTION_CODE(CLS, BASE, CODE) \
    class CLS: public BASE                                                           \
    {                                                                                \
    public:                                                                          \
        explicit CLS(int code = CODE);                                                        \
        explicit CLS(const std::string& msg, int code = CODE);                                \
        CLS(const std::string& msg, const std::string& arg, int code = CODE);        \
        CLS(const std::string& msg, const Exception& exc, int code = CODE);          \
        CLS(const CLS& exc);                                                         \
        ~CLS() override;                                                             \
        CLS& operator = (const CLS& exc);                                            \
        [[nodiscard]] const char* name() const noexcept override;                    \
        [[nodiscard]] const char* className() const noexcept override;               \
        [[nodiscard]] Exception* clone() const override;                             \
        void rethrow() const override;                                               \
    };

#define DECLARE_EXCEPTION(CLS, BASE) \
        DECLARE_EXCEPTION_CODE(CLS, BASE, 0)

#define IMPLEMENT_EXCEPTION(CLS, BASE, NAME)                                                         \
    CLS::CLS(int code): BASE(code) {                                                                 \
    }                                                                                                \
    CLS::CLS(const std::string& msg, int code): BASE(msg, code) {                                    \
    }                                                                                                \
    CLS::CLS(const std::string& msg, const std::string& arg, int code): BASE(msg, arg, code) {       \
    }                                                                                                \
    CLS::CLS(const CLS& exc): BASE(exc) {                                                            \
    }                                                                                                \
    CLS::~CLS() = default;                                                                           \
    CLS& CLS::operator = (const CLS& exc) {                                                          \
        BASE::operator = (exc);                                                                      \
        return *this;                                                                                \
    }                                                                                                \
    const char* CLS::name() const noexcept {                                                         \
        return NAME;                                                                                 \
    }                                                                                                \
    const char* CLS::className() const noexcept  {                                                   \
        return typeid(*this).name();                                                                 \
    }                                                                                                \
    Exception* CLS::clone() const {                                                                  \
        return new CLS(*this);                                                                       \
    }                                                                                                \
    void CLS::rethrow() const {                                                                      \
        throw *this;                                                                                 \
    }


//
// Standard exception classes
//
DECLARE_EXCEPTION(LogicException, Exception)
DECLARE_EXCEPTION(AssertionViolationException, LogicException)
DECLARE_EXCEPTION(NullPointerException, LogicException)
DECLARE_EXCEPTION(NullValueException, LogicException)
DECLARE_EXCEPTION(BugcheckException, LogicException)
DECLARE_EXCEPTION(InvalidArgumentException, LogicException)
DECLARE_EXCEPTION(NotImplementedException, LogicException)
DECLARE_EXCEPTION(RangeException, LogicException)
DECLARE_EXCEPTION(IllegalStateException, LogicException)
DECLARE_EXCEPTION(InvalidAccessException, LogicException)
DECLARE_EXCEPTION(SignalException, LogicException)
DECLARE_EXCEPTION(UnhandledException, LogicException)

DECLARE_EXCEPTION(RuntimeException, Exception)
DECLARE_EXCEPTION(NotFoundException, RuntimeException)
DECLARE_EXCEPTION(ExistsException, RuntimeException)
DECLARE_EXCEPTION(TimeoutException, RuntimeException)
DECLARE_EXCEPTION(SystemException, RuntimeException)
DECLARE_EXCEPTION(RegularExpressionException, RuntimeException)
DECLARE_EXCEPTION(LibraryLoadException, RuntimeException)
DECLARE_EXCEPTION(LibraryAlreadyLoadedException, RuntimeException)
DECLARE_EXCEPTION(NoThreadAvailableException, RuntimeException)
DECLARE_EXCEPTION(PropertyNotSupportedException, RuntimeException)
DECLARE_EXCEPTION(PoolOverflowException, RuntimeException)
DECLARE_EXCEPTION(NoPermissionException, RuntimeException)
DECLARE_EXCEPTION(OutOfMemoryException, RuntimeException)
DECLARE_EXCEPTION(DataException, RuntimeException)

DECLARE_EXCEPTION(DataFormatException, DataException)
DECLARE_EXCEPTION(SyntaxException, DataException)
DECLARE_EXCEPTION(CircularReferenceException, DataException)
DECLARE_EXCEPTION(PathSyntaxException, SyntaxException)
DECLARE_EXCEPTION(IOException, RuntimeException)
DECLARE_EXCEPTION(ProtocolException, IOException)
DECLARE_EXCEPTION(FileException, IOException)
DECLARE_EXCEPTION(FileExistsException, FileException)
DECLARE_EXCEPTION(FileNotFoundException, FileException)
DECLARE_EXCEPTION(PathNotFoundException, FileException)
DECLARE_EXCEPTION(FileReadOnlyException, FileException)
DECLARE_EXCEPTION(FileAccessDeniedException, FileException)
DECLARE_EXCEPTION(CreateFileException, FileException)
DECLARE_EXCEPTION(OpenFileException, FileException)
DECLARE_EXCEPTION(WriteFileException, FileException)
DECLARE_EXCEPTION(ReadFileException, FileException)
DECLARE_EXCEPTION(UnknownURISchemeException, RuntimeException)

DECLARE_EXCEPTION(ApplicationException, Exception)
DECLARE_EXCEPTION(BadCastException, RuntimeException)

#endif //CHATCONTROLLER_COMMON_EXCEPTION_H_

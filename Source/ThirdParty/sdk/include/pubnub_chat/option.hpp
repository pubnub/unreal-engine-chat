#ifndef PN_CHAT_OPTION_HPP
#define PN_CHAT_OPTION_HPP

#include <optional>
#include <functional>

namespace Pubnub {
    /** 
     * A type that represents an optional value.
     *
     * The Option type is a common way to represent the presence or absence of a value.
     * It is often used as the return type of functions that may or may not return a meaningful
     * value when the function doesn't throw an exception.
     *
     * The Option type is a wrapper around an optional value. It is a generic type, meaning that
     * it can be used with any type of value.
     *
     * Example:
     * @code
     * Option<int> some_value = Option<int>::some(42);
     * Option<int> no_value = Option<int>::none();
     *
     * if (some_value.has_value()) {
     *    std::cout << "The value is " << some_value.value() << std::endl;
     *    // Output: The value is 42
     * }
     *
     * if (no_value.has_value()) {
     *   std::cout << "The value is " << no_value.value() << std::endl;
     *   // This block will not be executed 
     * }
     * @endcode
     */
    template <typename T>
    class Option {
        public:
            /**
             * Creates an Option object that represents the absence of a value.
             */
            Option(): maybe(std::nullopt) {};

            /**
             * Creates an Option object that represents the presence of a value.
             *
             * @param value The value to be wrapped by the Option object.
             */
            Option(T value): maybe(value) {};

            /**
             * Gets an Option object that represents the presence of a value from
             * std::optional.
             *
             * @param value The optional value to be wrapped by the Option object.
             */
            Option(std::optional<T> value): maybe(value) {};

            /**
             * Creates an Option object that represents the absence of a value.
             */
            static Option<T> none() {
                return Option<T>();
            };

            /**
             * Creates an Option object that represents the presence of a value.
             *
             * @param value The value to be wrapped by the Option object.
             */
            static Option<T> some(T value) {
                return Option<T>(value);
            };

            /**
             * Function checks if the Option object contains a value.
             *
             * @return true if the Option object contains a value, false otherwise.
             *
             * Example:
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * Option<int> no_value = Option<int>::none();
             *
             * if (some_value.has_value()) {
             *   std::cout << "The value is " << some_value.value() << std::endl;
             *   // Output: The value is 42
             * }
             * @endcode
             */
            bool has_value() {
                return this->maybe.has_value();
            };

            /**
             * Function returns the value wrapped by the Option object.
             *
             * @return The value wrapped by the Option object.
             *
             * Example:
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * Option<int> no_value = Option<int>::none();
             *
             * if (some_value.has_value()) {
             *   std::cout << "The value is " << some_value.value() << std::endl;
             *   // Output: The value is 42
             * }
             * @endcode
             */
            T value() {
                return this->maybe.value();
            };

            /**
             * Function returns the value wrapped by the Option object or a default value if the Option object is empty.
             *
             * @param default_value The default value to be returned if the Option object is empty.
             *
             * @return The value wrapped by the Option object or the default value if the Option object is empty.
             *
             * Example:
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * Option<int> no_value = Option<int>::none();
             *
             * std::cout << "The value is " << some_value.value_or(0) << std::endl;
             * // Output: The value is 42
             *
             * std::cout << "The value is " << no_value.value_or(0) << std::endl;
             * // Output: The value is 0
             * @endcode
             */
            T value_or(T default_value) {
                return this->maybe.value_or(default_value);
            };

            /**
             * Function calls a function with the value wrapped by the Option object if the Option object is not empty.
             * If the Option object is empty, the function returns an empty Option object.
             *
             * @param f The function to be called with the value wrapped by the Option object.
             *
             * @return An Option object that contains the result of the function call or an empty Option object.
             *
             * Example:
             * @code
             * Option<int> some_value = Option<int>::some(42);
             *
             * Option<int> result = some_value.and_then([](int value) {
             *  return Option<int>::some(value * 2);
             * });
             *
             * if (result.has_value()) {
             *  std::cout << "The value is " << result.value() << std::endl;
             *  // Output: The value is 84
             * }
             * @endcode
             */
            Option<T> and_then(std::function<Option<T>(T)> f) {
                if (this->has_value()) {
                    return f(this->maybe());
                }

                return Option<T>::none();
            };

            /**
             * Function calls a function and returns the result of the function call if the Option object is empty.
             * If the Option object is not empty, the function returns the Option object.
             *
             * @param f The function to be called if the Option object is empty.
             *
             * @return The Option object if the Option object is not empty or the result of the function call.
             *
             * Example:
             * @code
             * Option<int> no_value = Option<int>::none();
             *
             * Option<int> result = no_value.or_else([]() {
             *  return Option<int>::some(0);
             * });
             *
             * if (result.has_value()) {
             *  std::cout << "The value is " << result.value() << std::endl;
             *  // Output: The value is 0
             * }
             * @endcode
             */
            Option<T> or_else(std::function<Option<T>()> f) {
                if (this->has_value()) {
                    return *this;
                }

                return f();
            };

            /**
             * Function transforms the value wrapped by the Option object using a function.
             *
             * Input callable is always called with the value wrapped by the Option object.
             *
             * @param f The function to be called with the value wrapped by the Option object.
             *
             * @return The Option object with the transformed value.
             *
             * Example:
             *
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * some_value.transform([](int value) {
             *  return value * 2;
             * });
             *
             * if (some_value.has_value()) {
             *  std::cout << "The value is " << some_value.value() << std::endl;
             *  // Output: The value is 84
             * }
             * @endcode
             */
            Option<T> transform(std::function<T(T)> f) {
                this->maybe = f(this->maybe);

                return *this;
            };

            /**
             * Function maps the value wrapped by the Option object using a function.
             *
             * Map checks if the Option object is empty. If it is not, 
             * it calls the function with the value wrapped by the Option object.
             *
             * @param f The function to be called with the value wrapped by the Option object.
             *
             * @return The Option object with the mapped value.
             *
             * Example:
             *
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * some_value.map([](int value) {
             *  return value * 2;
             * });
             *
             * if (some_value.has_value()) {
             *  std::cout << "The value is " << some_value.value() << std::endl;
             *  // Output: The value is 84
             * }
             * @endcode
             */
            Option<T> map(std::function<T(T)> f) {
                if (this->has_value()) {
                    return Option<T>::some(f(this->maybe()));
                }

                return Option<T>::none();
            };

            /**
             * Function filters the value wrapped by the Option object using a function.
             *
             * Filter checks if the Option object is empty. If it is not,
             * it calls the function with the value wrapped by the Option object.
             * If the function returns true, the Option object is returned.
             * Otherwise, an empty Option object is returned.
             *
             * @param f The function to be called with the value wrapped by the Option object.
             *
             * @return The Option object with the filtered value.
             *
             * Example:
             * @code
             * Option<int> some_value = Option<int>::some(42);
             * some_value.filter([](int value) {
             *  return value > 40;
             * });
             *
             * if (some_value.has_value()) {
             *  std::cout << "The value is " << some_value.value() << std::endl;
             *  // Output: The value is 42
             * }
             * @endcode
             */
            Option<T> filter(std::function<bool(T)> f) {
                if (this->has_value() && f(this->maybe())) {
                    return *this;
                }

                return Option<T>::none();
            };

            /**
             * Function returns a pointer to the value wrapped by the Option object.
             *
             * It is meant to be used over the C ABI but can be used for any reasons that 
             * require a pointer to the value wrapped by the Option object.
             *
             * @return A pointer to the value wrapped by the Option object or a null pointer.
             */
            T* c_ptr() {
                if (this->has_value()) {
                    return &this->maybe.value();
                }

                return nullptr;
            }

            /**
             * Function returns a pointer to the value wrapped by the Option object.
             *
             * It is meant to be used over the C ABI.
             *
             * It allocates memory for the value and returns a pointer to it if the Option object is not empty.
             * Otherwise, it returns a null pointer.
             *
             * @return A pointer to the value wrapped by the Option object or a null pointer.
             */
            T* into_c_pointer() {
                if (this->has_value()) {
                    return new T(this->maybe());
                }

                return nullptr;
            };
        private:
            std::optional<T> maybe;
    };
}

#endif // PN_CHAT_OPTION_HPP

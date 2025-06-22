#pragma once
#include <random>
#include <type_traits>
#include <thread>

namespace utils
{
    /**
     * @brief Универсальный генератор случайных чисел для любых арифметических типов.
     *
     * Класс RandomGenerator предоставляет потокобезопасный генератор случайных чисел,
     * поддерживающий любые арифметические типы (int, float, size_t и др.).
     * Позволяет генерировать случайные значения в заданном диапазоне, автоматически
     * приводя типы границ диапазона к целевому типу шаблона.
     */
    template<typename T>
    class RandomGenerator
    {
    public:
        /**
         * @brief Конструктор генератора.
         *
         * Проверяет, что тип T является арифметическим (целым или с плавающей точкой).
         */
        inline RandomGenerator<T>() {
            static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type (integral or floating-point)");
        };

        /**
         * @brief Получить случайное значение в диапазоне [min, max].
         *
         * @tparam U Тип минимального значения (любой арифметический тип).
         * @tparam V Тип максимального значения (любой арифметический тип).
         * @param min Минимальное значение диапазона.
         * @param max Максимальное значение диапазона.
         * @return Случайное значение типа T в заданном диапазоне.
         */
        template<typename U, typename V>
        inline T random(U min, V max)
        {
            static_assert(std::is_arithmetic_v<U> && std::is_arithmetic_v<V>, "Arguments must be arithmetic types");
            T tmin = static_cast<T>(min);
            T tmax = static_cast<T>(max);

            // Thread-local random engine with proper seeding
            thread_local std::mt19937 gen = [] {
                std::random_device rd;
                auto seed1 = static_cast<std::mt19937::result_type>(rd());
                auto seed2 = static_cast<std::mt19937::result_type>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
                return std::mt19937(seed1 ^ seed2);
                }();

            if constexpr (std::is_integral_v<T>)
            {
                std::uniform_int_distribution<T> dis(tmin, tmax);
                return dis(gen);
            }
            else
            {
                std::uniform_real_distribution<T> dis(tmin, tmax);
                return dis(gen);
            }
        }

        /**
         * @brief Оператор вызова для генерации случайного значения в диапазоне [min, max].
         *
         * @tparam U Тип минимального значения (любой арифметический тип).
         * @tparam V Тип максимального значения (любой арифметический тип).
         * @param min Минимальное значение диапазона.
         * @param max Максимальное значение диапазона.
         * @return Случайное значение типа T в заданном диапазоне.
         */
        template<typename U, typename V>
        inline T operator()(U min, V max)
        {
            return random(min, max);
        }
    };
}
#ifndef CPPAPE_LABEL_H
#define CPPAPE_LABEL_H

#include "baselib.h"
#include <vector>

namespace ape
{
	namespace detail
	{
		template<typename T>
		struct fmt_type_traits;

		template<>
		struct fmt_type_traits<signed int>
		{
			static constexpr const char* designator()
			{
				return "d";
			}
		};

		template<>
		struct fmt_type_traits<const char*>
		{
			static constexpr const char* designator()
			{
				return "s";
			}
		};

		template<>
		struct fmt_type_traits<void*>
		{
			static constexpr const char* designator()
			{
				return "x";
			}
		};

		template<>
		struct fmt_type_traits<unsigned int>
		{
			static constexpr const char* designator()
			{
				return "u";
			}
		};

		template<>
		struct fmt_type_traits<float>
		{
			static constexpr const char* designator()
			{
				return "f";
			}
		};

		template<>
		struct fmt_type_traits<double>
		{
			static constexpr const char* designator()
			{
				return "lf";
			}
		};

		template<>
		struct fmt_type_traits<char>
		{
			static constexpr const char* designator()
			{
				return "c";
			}
		};

		template<>
		struct fmt_type_traits<long double>
		{
			static constexpr const char* designator()
			{
				return "ld";
			}
		};

		template<>
		struct fmt_type_traits<unsigned long long>
		{
			static constexpr const char* designator()
			{
				return "lu";
			}
		};

		template<>
		struct fmt_type_traits<signed long long>
		{
			static constexpr const char* designator()
			{
				return "li";
			}
		};

		class ControlBlockBase
		{
		public:

			class Handle
			{
			public:
				friend class ControlBlockBase;

				~Handle()
				{
					parent->decRef();
				}

				Handle(Handle&& other) = default;
				Handle& operator = (Handle&& other) = default;

				Handle(const Handle& other)
					: parent(other.parent)
				{
					parent->addRef();
				}

				Handle& operator = (const Handle& other)
				{
					other.parent->addRef();
					parent->decRef();
					parent = other.parent;
					return *this;
				}

			private:

				Handle(ControlBlockBase& parent)
					: parent(&parent)
				{
					parent.addRef();
				}

				ControlBlockBase* parent;
			};

			virtual ~ControlBlockBase() {}

			Handle handle()
			{
				return { *this };
			}

		private:

			void addRef()
			{
				counter++;
			}

			void decRef()
			{
				if (--counter == 0)
				{
					delete this;
				}
			}


			std::size_t counter = 0;
		};

	}

	/// <summary>
	/// An assignable (from <typeparamref name="T"/>) value to be used as arguments for a <see cref="Label"/>.
	/// You should store these separately, and when you assign to them the linked <see cref="Label"/> will get updated.
	/// </summary>
	/// <typeparam name="T">
	/// A primitive, scalar type (integers, floating point types or pointer).
	/// </typeparam>
	template<typename T>
	class SharedValue
	{
	public:

		SharedValue(const T value = T())
			: memory(new ControlBlock())
			, handle(memory->handle())
		{

		}

		/// <summary>
		/// Write to the value contained.
		/// </summary>
		SharedValue<T>& operator = (const T& value)
		{
			*getPtr() = value;
			return *this;
		}

		/// <summary>
		/// Read the value contained.
		/// </summary>
		operator T ()
		{
			return *getPtr();
		}

		/// <summary>
		/// Create a handle to this value.
		/// The value will be kept alive until all the handles go out of scope.
		/// </summary>
		/// <returns></returns>
		detail::ControlBlockBase::Handle createHandle()
		{
			return memory->handle();
		}

		/// <summary>
		/// Retrieve a pointer to the shared value.
		/// Used internally.
		/// </summary>
		T* getPtr()
		{
			return &memory->value;
		}

		/// <summary>
		/// Used internally.
		/// </summary>
		const char* getTypeDesignator()
		{
			return detail::fmt_type_traits<T>::designator();
		}

	private:

		class ControlBlock : public detail::ControlBlockBase
		{
		public:
			T value;
		};

		ControlBlock* memory;
		detail::ControlBlockBase::Handle handle;
	};

	/// <summary>
	/// A <see cref="Label"/> is a printf-like format string displayed to the user, that is automatically 
	/// updated in the GUI.
	/// <seealso cref="print()"/>
	/// <seealso cref="SharedValue"/>
	/// </summary>
	class Label : public UIObject
	{
	public:

		/// <summary>
		/// Construct a label.
		/// </summary>
		/// <param name="name">
		/// The title of the label.
		/// </param>
		/// <param name="fmt">
		/// A format string where the nth % char is substituded for the nth <see cref="SharedValue"/> in the following
		/// <paramref name="args"/>. Use two %% to display a single %.
		/// <seealso cref="print()"/>
		/// </param>
		/// <param name="args">
		/// A list of <see cref="SharedValue"/> to be displayed in the <paramref name="fmt"/>
		/// </param>
		template<typename... Args>
		Label(const std::string& name, const std::string_view fmt, Args&... args)
		{
			values = { args.createHandle()... };
			const char* types[] = { args.getTypeDesignator()... };

			std::string temp;
			auto numTypes = std::extent<decltype(types)>::value;
			temp.reserve(fmt.size() + numTypes);

			for (std::size_t i = 0, c = 0; i < fmt.size(); ++i)
			{
				temp += fmt[i];

				if (fmt[i] == '%')
				{
					if (i + 1 < fmt.size() && fmt[i + 1] == '%')
					{
						continue;
					}
					else if (c < numTypes)
					{
						temp += types[c++];
					}
					else
					{
						abort("Invalid format specifier + argument list combination");
					}
				}

			}

			id = getInterface().createLabel(&getInterface(), name.c_str(), temp.c_str(), args.getPtr()...);
		}

		~Label()
		{
			getInterface().destroyResource(&getInterface(), id, 0);
		}

	private:

		int id;
		std::vector<detail::ControlBlockBase::Handle> values;
	};

}
#endif
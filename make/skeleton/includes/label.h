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

	template<typename T>
	class SharedValue
	{
	public:

		SharedValue(const T value = T())
			: memory(new ControlBlock())
			, handle(memory->handle())
		{

		}

		SharedValue<T>& operator = (const T& value)
		{
			*getPtr() = value;
			return *this;
		}

		operator T ()
		{
			return *getPtr();
		}

		detail::ControlBlockBase::Handle createHandle()
		{
			return memory->handle();
		}

		T* getPtr()
		{
			return &memory->value;
		}

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

	class Label
	{
	public:

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
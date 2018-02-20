/*************************************************************************************

	C++ compiler for Audio Programming Environment. 

    Copyright (C) 2017 Janus Lynggaard Thorborg [LightBridge Studios]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	See \licenses\ for additional details on licenses associated with this program.

**************************************************************************************

	file:TranslationUnit.h
		
		Class that handles a complete translation unit.

*************************************************************************************/
#ifndef CPPAPE_TRANSLATIONUNIT_H
#define CPPAPE_TRANSLATIONUNIT_H
#include <cpl/Misc.h>
#include <cpl/Process.h>
#include <experimental/filesystem>
#include "libCppJit.h"
#include <memory>

namespace CppAPE
{
	class LibCppJitExceptionBase : public std::exception
	{
	public:

		LibCppJitExceptionBase(jit_error_t error) : error(error) {}

		jit_error_t error;

		const char * what() const override
		{
			return jit_format_error(error);
		}
	};

	class CxxJitContext;

	class CxxTranslationUnit
	{
		typedef translation_unit InternalUnit;
		friend class CxxJitContext;
	public:

		class CompilationException : public LibCppJitExceptionBase { using LibCppJitExceptionBase::LibCppJitExceptionBase; };

		class Builder : private cpl::Args
		{
			friend class CxxTranslationUnit;
			typedef std::function<void(jit_error_t errorType, const char * msg)> ErrorCallback;
		public:

			CxxTranslationUnit fromString(const cpl::string_ref contents, const cpl::string_ref name = "")
			{
				trs_unit_options options{};
				options.size = sizeof(trs_unit_options);
				options.argc = (int)argc();
				if(argc())
					options.argv = argv();

				if (name.size())
					options.name = name.c_str();

				options.opaque = this;
				options.callback = onCompiler;

				translation_unit* localUnit(nullptr);

				auto ret = trs_unit_from_string(contents.c_str(), &options, &localUnit);

				if (ret != jit_error_none)
				{
					throw CompilationException{ ret };
				}

				return { localUnit };
			}

			CxxTranslationUnit fromFile(const cpl::string_ref contents)
			{
				trs_unit_options options{};
				options.size = sizeof(trs_unit_options);
				options.argc = (int)argc();
				if (argc())
					options.argv = argv();

				options.opaque = this;
				options.callback = onCompiler;

				translation_unit* localUnit(nullptr);

				if (auto ret = trs_unit_from_file(contents.c_str(), &options, &localUnit); ret != jit_error_none)
				{
					throw CompilationException{ ret };
				}

				return { localUnit };
			}

			Builder& includeDirs(const std::vector<std::string>& dirs)
			{
				for (auto& dir : dirs)
					argPair("-I", dir, NoSpace | Escaped);

				return *this;
			}

			Builder& onMessage(ErrorCallback onMessageCallback)
			{
				callback = std::move(onMessageCallback);
				return *this;
			}

			cpl::Args& args()
			{
				return *this;
			}

		private:

			static void onCompiler(void* op, const char* msg, jit_error_t error)
			{
				if (Builder* b = static_cast<Builder*>(op))
				{
					if (b->callback)
						b->callback(error, msg);
				}
			}

			ErrorCallback callback;
		};

		static CxxTranslationUnit loadSaved(cpl::string_ref where)
		{
			translation_unit* unit = nullptr;
			if (auto ret = trs_unit_load_saved(where.c_str(), &unit); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}

			return { unit };
		}

		void save(cpl::string_ref where) const
		{
			if (auto ret = trs_unit_save(unit.get(), where.c_str()); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}
		
	private:

		CxxTranslationUnit(InternalUnit* ptr)
			: unit(ptr)
		{

		}

		class UnitDeleter
		{
		public:
			void operator () (InternalUnit* unit)
			{
				if (unit)
					trs_unit_delete(unit);
			}
		};

		std::unique_ptr<InternalUnit, UnitDeleter> unit;
	};

	class CxxJitContext
	{
		typedef jit_context InternalContext;

	public:

		typedef std::function<void(jit_error_t errorType, const char * msg)> ErrorCallback;

		CxxJitContext()
		{
			jit_context* localCtx;
			if (auto ret = jit_create_context(&localCtx); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}

			ctx.reset(localCtx);
		}

		template<typename T>
		T* getGlobal(const cpl::string_ref name)
		{
			void * loc;
			if (auto ret = jit_get_symbol(ctx.get(), name.c_str(), &loc); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}

			return reinterpret_cast<T*>(loc);
		}

		template<typename T>
		T getFunction(const cpl::string_ref name)
		{
			void * loc;
			if (auto ret = jit_get_symbol(ctx.get(), name.c_str(), &loc); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}

			return reinterpret_cast<T>(loc);
		}

		template<typename T>
		void injectSymbol(const cpl::string_ref name, T* location)
		{
			if (auto ret = jit_inject_symbol(ctx.get(), location); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}

		void addTranslationUnit(const CxxTranslationUnit& unit)
		{
			if (auto ret = jit_add_translation_unit(ctx.get(), unit.unit.get()); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}

		void finalize()
		{
			if (auto ret = jit_finalize(ctx.get()); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}

		void setCallback(ErrorCallback cb)
		{
			callback = std::move(cb);
			jit_set_callback(ctx.get(), this, onError);
		}

		void openRuntime()
		{
			if (auto ret = jit_open(ctx.get()); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}

		void closeRuntime()
		{
			if (auto ret = jit_open(ctx.get()); ret != jit_error_none)
			{
				throw LibCppJitExceptionBase{ ret };
			}
		}

	private:

		class CtxDeleter
		{
		public:
			void operator () (InternalContext* unit)
			{
				if (unit)
					jit_delete_context(unit);
			}
		};

		static void onError(void* op, const char* msg, jit_error_t error)
		{
			if (CxxJitContext* c = static_cast<CxxJitContext*>(op))
			{
				if (c->callback)
					c->callback(error, msg);
			}
		}
		ErrorCallback callback;
		std::unique_ptr<InternalContext, CtxDeleter> ctx;
	};
};


#endif
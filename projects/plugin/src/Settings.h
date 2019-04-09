/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:Settings.h
		Includes settings system.

*************************************************************************************/

#ifndef APE_SETTINGS_H
	#define APE_SETTINGS_H

	#include <cpl/MacroConstants.h>

	#ifdef CPL_MSVC

		#pragma warning (disable: 4275)
		#pragma warning (disable: 4290)

	#endif

	#include <libconfig.hh>
	#include "Common.h"
	#include <cpl/Core.h>
	#include <filesystem>
	#include <optional>
	#include <set>

	namespace ape
	{
		class Settings
		{
		public:

			class Listener
			{
			public:
				virtual void onSettingsChanged(const Settings& parent, const libconfig::Setting& setting) = 0;
				virtual ~Listener() {}
			};
			
			Settings(bool saveOnDestruction, cpl::fs::path path)
				: saveOnDestruction(saveOnDestruction), path(std::move(path))
			{
				reloadSettings();
			}

			~Settings()
			{
				if (saveOnDestruction)
				{
					saveSettings();
				}
			}

			void addListener(Listener * list)
			{
				listeners.emplace(list);
			}

			void removeListener(Listener * list)
			{
				listeners.erase(list);
			}

			void changed(libconfig::Setting& s)
			{
				for (auto listener : listeners)
					listener->onSettingsChanged(*this, s);
			}

			const cpl::fs::path& getPath() const noexcept
			{
				return path;
			}

			const libconfig::Setting& root() const noexcept
			{
				return config.getRoot();
			}

			libconfig::Setting& root() 
			{
				return config.getRoot();
			}

			const std::optional<std::string>& getErrors() const noexcept
			{
				return lastErrorMessage;
			}

			void saveSettings()
			{
				config.writeFile(path.string().c_str());
			}

			template<typename T, typename... Paths>
			T lookUpValue(const T& defaultValue, Paths&&... paths) const
			{
				const char* compiledPaths[] = { paths... };

				try
				{
					const libconfig::Setting* setting = &config.getRoot();
					for (const auto& path : compiledPaths)
					{
						if (!setting->exists(path))
							return defaultValue;
						else
							setting = &setting->lookup(path);
					}

					return (T)*setting;
				}
				catch (const libconfig::SettingNotFoundException&)
				{
					return defaultValue;
				}
			}

			template<typename... Paths>
			std::string lookUpValue(std::string_view defaultValue, Paths&&... paths) const
			{
				const char* compiledPaths[] = { paths... };

				try
				{
					const libconfig::Setting* setting = &config.getRoot();
					for (const auto& path : compiledPaths)
					{
						if (!setting->exists(path))
							return std::string{ defaultValue };
						else
							setting = &setting->lookup(path);
					}

					return setting->c_str();
				}
				catch (const libconfig::SettingNotFoundException&)
				{
					return std::string { defaultValue };
				}
			}

			template<typename... Paths>
			juce::Colour lookUpValue(juce::Colour defaultValue, Paths&&... paths) const
			{
				const char* compiledPaths[] = { paths... };

				try
				{
					const libconfig::Setting* setting = &config.getRoot();
					for (const auto& path : compiledPaths)
					{
						if (!setting->exists(path))
							return defaultValue;
						else
							setting = &setting->lookup(path);
					}

					if (setting->getType() != libconfig::Setting::TypeString)
						return defaultValue;

					return juce::Colour::fromString(setting->c_str());

				}
				catch (const libconfig::SettingNotFoundException&)
				{
					return defaultValue;
				}
			}

			void reloadSettings()
			{
				try 
				{
					config.readFile(path.string().c_str());

					libconfig::Setting & approot = root()["application"];

					auto ensure = [&approot](const auto& s, auto def, auto type)
					{
						if (!approot.exists(s))
							approot.add(s, type);
					};

					ensure("log_console", false, libconfig::Setting::TypeBoolean);
					ensure("console_std_writing", false, libconfig::Setting::TypeBoolean);
					ensure("use_fpe", false, libconfig::Setting::TypeBoolean);
					ensure("ui_refresh_interval", 50, libconfig::Setting::TypeInt);
					ensure("autosave_interval", 60, libconfig::Setting::TypeInt);
					ensure("unique_id", 'apeX', libconfig::Setting::TypeInt);
					ensure("greeting_shown", true, libconfig::Setting::TypeBoolean);
					ensure("render_opengl", false, libconfig::Setting::TypeBoolean);

				}
				catch (libconfig::FileIOException & e)
				{
					lastErrorMessage = cpl::format("Error reading config file (%s)! (%s)", path.string().c_str(), e.what());
				}
				catch (libconfig::SettingNotFoundException & e)
				{
					lastErrorMessage = cpl::format("Error getting setting! (%s)", e.getPath());
				}
				catch (libconfig::ParseException & e)
				{
					lastErrorMessage = cpl::format("Error parsing config! In file %s at line %d: %s", e.getFile(), e.getLine(), e.getError());
				}
				catch (std::exception & e)
				{
					lastErrorMessage = cpl::format("Unknown error occured while reading settings! (%s)", e.what());
				}

			}

		private:

			std::set<Listener*> listeners;
			bool saveOnDestruction;
			std::optional<std::string> lastErrorMessage;
			libconfig::Config config;
			cpl::fs::path path;
		};

		inline bool operator == (const libconfig::Setting& a, const libconfig::Setting& b)
		{
			return &a == &b;
		}

		inline bool operator != (const libconfig::Setting& a, const libconfig::Setting& b)
		{
			return &a != &b;
		}

	}
	
#endif
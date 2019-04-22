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

	file:SourceFile.h
	
		Represents an immutable source file (except for serialization)

*************************************************************************************/

#ifndef APE_SOURCEFILE_H
	#define APE_SOURCEFILE_H

	#include <cpl/Common.h>
	#include <cpl/state/Serialization.h>
	#include <cpl/Core.h>
	#include <memory>

	namespace ape
	{
		namespace fs = cpl::fs;

		class SourceFile final : public cpl::CSerializer::Serializable
		{
		public:

			class Listener
			{
			public:
				virtual void fileChanged(const SourceFile& file) = 0;
				virtual ~Listener() {}
			};

			SourceFile(fs::path pathToUse);
			
			SourceFile(SourceFile&& other);
			SourceFile& operator = (SourceFile&& other);

			~SourceFile();

			const fs::path& getPath() const noexcept
			{
				return path;
			}

			std::string getName() const noexcept
			{
				return path.filename().string();
			}

			std::string getExtension() const noexcept
			{
				if (path.has_extension())
					return path.extension().string().substr(1);

				return "";
			}

			fs::path getDirectory() const noexcept
			{
				return path.parent_path().string();
			}

			bool isActualFile() const noexcept
			{
				return isFile;
			}

			SourceFile asNonExisting() const noexcept
			{
				return { path, false };
			}

			static SourceFile asNonExisting(fs::path path)
			{
				return { std::move(path), false };
			}

			static SourceFile untitled()
			{
				return { "untitled", false };
			}

			juce::File getJuceFile() const noexcept
			{
				return { path.string() };
			}

			void addListener(Listener& list);
			void removeListener(Listener& list);

		protected:

			void serialize(cpl::CSerializer::Archiver & ar, cpl::Version version) override
			{
				ar << path.string() << isFile;
			}

			void deserialize(cpl::CSerializer::Builder & builder, cpl::Version version) override
			{
				std::string stringPath;
				builder >> stringPath;

				path = stringPath;

				bool isActualFile;

				builder >> isActualFile;

				isFile = isActualFile && fs::exists(path);
			}

		private:

			void onFileChanged();

			class ListenerManager;
			ListenerManager& getListenerManager();

			SourceFile(fs::path pathToUse, bool isActualFile);

			fs::path path;
			bool isFile;

			std::unique_ptr<ListenerManager> listenerManager;
		};
	}
#endif
#ifndef COMMON_HELPER_H
	#define COMMON_HELPER_H

	#include <ape/SharedInterface.h>
	#include <experimental/filesystem>

	namespace tests
	{
		namespace fs = std::experimental::filesystem;

		inline fs::path RepositoryRoot()
		{
			fs::path current = fs::current_path();
			while (current.has_parent_path())
			{
				if (current.filename() == "ape")
					return current;

				current = current.parent_path();
			}

			throw std::runtime_error("could not find root repository");
		}
	};

#endif
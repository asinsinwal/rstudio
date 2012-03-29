/*
 * HunspellDictionaryManager.cpp
 *
 * Copyright (C) 2009-11 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include <core/spelling/HunspellDictionaryManager.hpp>

#include <core/Algorithm.hpp>

namespace core {
namespace spelling {
namespace hunspell {

namespace {

struct KnownDictionary
{
   const char* id;
   const char* name;
} ;

KnownDictionary s_knownDictionaries[] =
{
   { "bg_BG",     "Bulgarian"                },
   { "ca_ES",     "Catalan"                  },
   { "cs_CZ",     "Czech"                    },
   { "da_DK",     "Danish"                   },
   { "de_DE",     "German"                   },
   { "de_DE_neu", "German (New)"             },
   { "el_GR",     "Greek"                    },
   { "en_AU",     "English (Australia)"      },
   { "en_CA",     "English (Canada)"         },
   { "en_GB",     "English (United Kingdom)" },
   { "en_US",     "English (United States)"  },
   { "es_ES",     "Spanish"                  },
   { "fr_FR",     "French"                   },
   { "hr_HR",     "Croatian"                 },
   { "hu-HU",     "Hungarian"                },
   { "id_ID",     "Indonesian"               },
   { "it_IT",     "Italian"                  },
   { "lt_LT",     "Lithuanian"               },
   { "lv_LV",     "Latvian"                  },
   { "nb_NO",     "Norwegian"                },
   { "nl_NL",     "Dutch"                    },
   { "pl_PL",     "Polish"                   },
   { "pt_BR",     "Portuguese (Brazil)"      },
   { "pt_PT",     "Portuguese (Portugal)"    },
   { "ro_RO",     "Romanian"                 },
   { "ru_RU",     "Russian"                  },
   { "sh",        "Serbo-Croatian"           },
   { "sk_SK",     "Slovak"                   },
   { "sl_SI",     "Slovenian"                },
   { "sr",        "Serbian"                  },
   { "sv_SE",     "Swedish"                  },
   { "uk_UA",     "Ukrainian"                },
   { "vi_VN",     "Vietnamese"               },
   { NULL, NULL }
};

FilePath dicPathForAffPath(const FilePath& affPath)
{
   return affPath.parent().childPath(affPath.stem() + ".dic");
}

bool isDictionaryAff(const FilePath& filePath)
{
   return (filePath.extensionLowerCase() == ".aff") &&
          dicPathForAffPath(filePath).exists();
}

Dictionary fromAffFile(const FilePath& filePath)
{
   return Dictionary(filePath);
}

Error listAffFiles(const FilePath& baseDir, std::vector<FilePath>* pAffFiles)
{
   if (!baseDir.exists())
      return Success();

   std::vector<FilePath> children;
   Error error = baseDir.children(&children);
   if (error)
      return error;

   core::algorithm::copy_if(children.begin(),
                            children.end(),
                            std::back_inserter(*pAffFiles),
                            isDictionaryAff);

   return Success();
}

bool compareByName(const Dictionary& dict1, const Dictionary& dict2)
{
   return dict1.name() < dict2.name();
}

} // anonymous namespace


std::string Dictionary::name() const
{
   std::string dictId = id();
   for (KnownDictionary* dict = s_knownDictionaries; dict->name; ++dict)
   {
      if (dictId == dict->id)
         return dict->name;
   }

   return dictId;
}

FilePath Dictionary::dicPath() const
{
   return dicPathForAffPath(affPath_);
}

Error DictionaryManager::availableLanguages(
                        std::vector<Dictionary>* pDictionaries) const
{
   // first try the user languages dir
   std::vector<FilePath> affFiles;

   if (allLanguagesInstalled())
   {
      Error error = listAffFiles(allLanguagesDir(), &affFiles);
      if (error)
         return error;
   }
   else
   {
      Error error = listAffFiles(coreLanguagesDir_, &affFiles);
      if (error)
         return error;
   }

   // always check the languages-extra directory as well (and auto-create
   // it so users who look for it will see it)
   FilePath userLangsDir = userLanguagesDir();
   Error error = userLangsDir.ensureDirectory();
   if (error)
      LOG_ERROR(error);
   error = listAffFiles(userLangsDir, &affFiles);
   if (error)
      LOG_ERROR(error);

   // convert to dictionaries
   std::transform(affFiles.begin(),
                  affFiles.end(),
                  std::back_inserter(*pDictionaries),
                  fromAffFile);

   // sort them by name
   std::sort(pDictionaries->begin(), pDictionaries->end(), compareByName);

   return Success();
}

Dictionary DictionaryManager::dictionaryForLanguageId(
                                       const std::string& langId) const
{
   std::string affFile = langId + ".aff";

   // first check to see whether it exists in the user languages directory
   FilePath userLangsAff = userLanguagesDir().complete(affFile);
   if (userLangsAff.exists())
      return Dictionary(userLangsAff);
   else if (allLanguagesInstalled())
      return Dictionary(allLanguagesDir().complete(affFile));
   else
      return Dictionary(coreLanguagesDir_.complete(affFile));
}

FilePath DictionaryManager::allLanguagesDir() const
{
   return userDir_.childPath("languages-system");
}

FilePath DictionaryManager::userLanguagesDir() const
{
   return userDir_.childPath("languages-user");
}

} // namespace hunspell
} // namespace spelling
} // namespace core

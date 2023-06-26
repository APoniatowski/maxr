/** class Catalog
    \file Catalog.h
    \author OOTA, Masato
    \copyright Copyright © 2019, 2022 OOTA, Masato
    \par License Boost
    \parblock
      This program is distributed under the Boost Software License Version 1.0.
      You can get the license file at “https://www.boost.org/LICENSE_1_0.txt”.
    \endparblock
*/

#ifndef SPIRITLESS_PO_CATALOG_H_
#define SPIRITLESS_PO_CATALOG_H_

#include "Common.h"
#include "MetadataParser.h"
#include "PluralParser.h"
#include "PoParser.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spiritless_po {
    /** Class Catalog handles a catalog that contains some original and translated messages.

        You need only to use this class and not directly to use other classes under include/spiritless_po/.

        This class has some features:
        - An instance of Catalog handles only one textdomain and only one language, doesn't handle multiple textdomains and multiple languages.
        - Catalog can read the messages from multiple PO files, instead of a single MO file. You can add new messages to a single catalog any number of times.
        - Catalog doesn't care the locale.
        - Catalog doesn't handle the character encoding.

        If you would use multiple textdomains and/or multiple languages, you need to use multiple instances of this class.
    */
    class Catalog {
    public:
        /** Create an empty catalog. */
        Catalog();

        /** Create and Add(begin, end).
            \tparam INP A type of an input iterator.
            \param [in] begin An input iterator pointing to the beginning of the range.
            \param [in] end An input iterator pointing to the end of the range.
        */
        template <class INP>
        explicit Catalog(INP begin, INP end);

        /** Create and Add(is).
            \param [in] is An input stream that contains PO entries.
        */
        explicit Catalog(std::istream &is);

        /** This class is copyable.
            \param [in] a The source.
        */
        Catalog(const Catalog &a) = default;

        /** This class is movable.
            \param [in] a The source.
        */
        Catalog(Catalog &&a) = default;

        /** This class is destructible. */
        ~Catalog() = default;

        /** This class is assignable.
            \param [in] a The source.
        */
        Catalog &operator=(const Catalog &a) = default;

        /** This class is move assignable.
            \param [in] a The source.
        */
        Catalog &operator=(Catalog &&a) = default;

        /** Clear all information and create an empty catalog. */
        void Clear();

        /** Add PO entries.
            \tparam INP A type of an input iterator.
            \param [in] begin An input iterator pointing to the beginning of the range.
            \param [in] end An input iterator pointing to the end of the range.
            \return true if no error is existed.
            \note This function doesn't change any existed entries, that is a translated text (msgstr) that corresponds to an existed original text (msgid), and also metadata if it's already existed.
            \note An entry isn't added if the msgstr (including msgstr[0]) is empty.
        */
        template <class INP>
        bool Add(INP begin, INP end);

        /** Add some PO entries.
            \param [in] is An input stream that contains PO entries.
            \return true if no error is existed.
            \note This function doesn't change any existed entries, that is a translated text (msgstr) that corresponds to an existed original text (msgid), and also metadata if it's already existed.
            \note An entry isn't added if the msgstr (including msgstr[0]) is empty.
        */
        bool Add(std::istream &is);

        /** Add another catalog contents.
            \param [in] a A catalog to add the entries.
            \note This function doesn't change any existed entries, that is a translated text (msgstr) that corresponds to an existed original text (msgid), and also metadata if it's already existed.
        */
        void Merge(const Catalog &a);

        /** Clear the error information. */
        void ClearError();

        /** Get the error information generated by Add() after ClearError() is called.
            \return The strings that describe the errors.
            \note The size of the result is 0 if no error is occurred.
        */
        const std::vector<std::string> &GetError() const;

        /** Get the translated text.
            \param [in] msgid The original text.
            \return The translated text if exists. If not, returns msgid. (It's a reference to the same object.)
        */
        const std::string &gettext(const std::string &msgid) const;

        /** Get the translated text.
            \param [in] msgid The original text.
            \param [in] msgidPlural The plural form of the original text.
            \param [in] n The value relating to the text.
            \return The translated text if exists. If not, returns msgid (if n == 1) or msgidPlural (if n != 1). (It's a reference to the same object.)
        */
        const std::string &ngettext(const std::string &msgid, const std::string &msgidPlural,
            unsigned long int n) const;

        /** Get the translated text.
            \param [in] msgctxt The context of the text.
            \param [in] msgid The original text.
            \return The translated text if exists. If not, returns msgid. (It's the reference to the same object.)
        */
        const std::string &pgettext(const std::string &msgctxt, const std::string &msgid) const;

        /** Get the translated text.
            \param [in] msgctxt The context of the text.
            \param [in] msgid The original text.
            \param [in] msgidPlural The plural form of the original text.
            \param [in] n The value relating to the text.
            \return The translated text if exists. If not, returns msgid (if n == 1) or msgidPlural (if n != 1). (It's a reference to the same object.)
        */
        const std::string &npgettext(const std::string &msgctxt, const std::string &msgid,
            const std::string &msgidPlural, unsigned long int n) const;


        /** Type of the string index.

            - stringTable[indexData[msgctxt + msgid].stringTableIndex] == msgstr
            - stringTable[indexData[msgctxt + msgid].stringTableIndex + n] == msgstr[n]
            - The maximum n is totalPlurals - 1.
            \attention This type is public to use for debugging and managing.
        */
        struct IndexDataT {
            std::size_t stringTableIndex; /**< The index of the StringTable. */
            std::size_t totalPlurals; /**< The number of the strings, including the plural forms, corresponding a msgid. */
        };

    private:
        MetadataParser::MapT metadata;
        std::unordered_map<std::string, IndexDataT> index;
        std::vector<std::string> stringTable;
        PluralParser::FunctionType pluralFunction;
        std::size_t maxPlurals;
        std::vector<std::string> errors;

    public:
        // Debugging and managing functions

        /** Get the metadata.
            \return The map of the metadata.
            \attention This function is public to use for debugging and managing.
         */
        const MetadataParser::MapT &GetMetadata() const;

        /** Get the index map.
            \return The map of the string index.
            \attention This function is public to use for debugging and managing.
            \note The size of the index map is the number of the translatable msgid.
         */
        const std::unordered_map<std::string, IndexDataT> &GetIndex() const;

        /** Get the string table.
            \return The string table.
            \attention This function is public to use for debugging and managing.
         */
        const std::vector<std::string> &GetStringTable() const;
    };

    inline Catalog::Catalog()
        : metadata(), index(), stringTable(), pluralFunction(),
          maxPlurals(0), errors()
    {
    }

    template <class INP>
    inline Catalog::Catalog(const INP begin, const INP end)
        : Catalog()
    {
        Add(begin, end);
    }

    inline Catalog::Catalog(std::istream &is)
        : Catalog()
    {
        Add(is);
    }

    inline void Catalog::Clear()
    {
        *this = Catalog();
    }

    template <class INP>
    bool Catalog::Add(const INP begin, const INP end)
    {
        std::vector<PoParser::PoEntryT> newEntries(PoParser::GetEntries(begin, end));
        for (auto &it : newEntries) {
            if (!it.error.empty()) {
                errors.push_back(std::move(it.error));
            } else if (!it.msgstr[0].empty()) {
                if (!it.msgid.empty()) {
                    if (index.find(it.msgid) == index.end()) {
                        IndexDataT idx;
                        idx.stringTableIndex = stringTable.size();
                        idx.totalPlurals = it.msgstr.size();
                        stringTable.insert(stringTable.end(), std::make_move_iterator(it.msgstr.begin()), std::make_move_iterator(it.msgstr.end()));
                        index.emplace(it.msgid, idx);
                    }
                } else if (metadata.empty()) {
                    metadata = MetadataParser::Parse(it.msgstr[0]);
                    const auto plural = metadata.find("Plural-Forms");
                    if (plural != metadata.end()) {
                        const auto pluralText = plural->second;
                        try {
                            const auto pluralData = PluralParser::Parse(pluralText);
                            if (pluralData.first > 0)
                                maxPlurals = pluralData.first - 1;
                            pluralFunction = pluralData.second;
                        } catch (PluralParser::ExpressionError &e) {
                            const size_t col = std::distance(pluralText.cbegin(), e.Where());
                            errors.emplace_back("Column#" + std::to_string(col + 1)
                                + " in plural expression: " + e.what());
                        }
                    }
                }
            }
        }
        return errors.empty();
    }

    inline bool Catalog::Add(std::istream &is)
    {
        std::istreambuf_iterator<char> begin(is);
        std::istreambuf_iterator<char> end;
        return Add(begin, end);
    }

    inline void Catalog::Merge(const Catalog &a)
    {
        if (metadata.empty()) {
            metadata = a.metadata;
            maxPlurals = a.maxPlurals;
            pluralFunction = a.pluralFunction;
        }
        for (const auto &src : a.index) {
            if (index.find(src.first) == index.end()) {
                auto srcIndexData = src.second;
                auto srcIndex = srcIndexData.stringTableIndex;
                auto srcTotal = srcIndexData.totalPlurals;
                auto srcStringIt = a.stringTable.begin() + srcIndex;
                IndexDataT idx;
                idx.stringTableIndex = stringTable.size();
                idx.totalPlurals = srcTotal;
                stringTable.insert(stringTable.end(), srcStringIt, srcStringIt + srcTotal);
                index.emplace(src.first, idx);
            }
        }
        errors.insert(errors.end(), a.errors.begin(), a.errors.end());
    }

    inline void Catalog::ClearError()
    {
        errors.clear();
    }

    inline const std::vector<std::string> &Catalog::GetError() const
    {
        return errors;
    }

    inline const std::string &Catalog::gettext(const std::string &msgid) const
    {
        const auto &it = index.find(msgid);
        if (it != index.end()) {
            return stringTable[it->second.stringTableIndex];
        } else {
            return msgid;
        }
    }

    inline const std::string &Catalog::ngettext(const std::string &msgid, const std::string &msgidPlural,
        unsigned long int n) const
    {
        const auto &it = index.find(msgid);
        if (it != index.end()) {
            std::size_t nIdx = pluralFunction(n);
            if (nIdx >= it->second.totalPlurals) {
                nIdx = 0;
            }
            return stringTable[it->second.stringTableIndex + nIdx];
        } else {
            if (n == 1) {
                return msgid;
            } else {
                return msgidPlural;
            }
        }
    }

    inline const std::string &Catalog::pgettext(const std::string &msgctxt, const std::string &msgid) const
    {
        std::string s(msgctxt);
        s += CONTEXT_SEPARATOR;
        s += msgid;
        const auto &it = index.find(s);
        if (it != index.end()) {
            return stringTable[it->second.stringTableIndex];
        } else {
            return msgid;
        }
    }

    inline const std::string &Catalog::npgettext(const std::string &msgctxt, const std::string &msgid,
        const std::string &msgidPlural, unsigned long int n) const
    {
        std::string s(msgctxt);
        s += CONTEXT_SEPARATOR;
        s += msgid;
        const auto &it = index.find(s);
        if (it != index.end()) {
            std::size_t nIdx = pluralFunction(n);
            if (nIdx >= it->second.totalPlurals)
                nIdx = 0;
            return stringTable[it->second.stringTableIndex + nIdx];
        } else {
            if (n == 1) {
                return msgid;
            } else {
                return msgidPlural;
            }
        }
    }

    inline const MetadataParser::MapT &Catalog::GetMetadata() const
    {
        return metadata;
    }

    inline const std::unordered_map<std::string, Catalog::IndexDataT> &Catalog::GetIndex() const
    {
        return index;
    }

    inline const std::vector<std::string> &Catalog::GetStringTable() const
    {
        return stringTable;
    }
} // namespace spiritless_po

#endif // SPIRITLESS_PO_CATALOG_H_

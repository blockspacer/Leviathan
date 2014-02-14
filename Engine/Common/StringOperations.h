#ifndef LEVIATHAN_STRINGOPERATIONS
#define LEVIATHAN_STRINGOPERATIONS
// ------------------------------------ //
#ifndef LEVIATHAN_DEFINE
#include "Define.h"
#endif
// ------------------------------------ //
// ---- includes ---- //


namespace Leviathan{

	//! \brief Helper class that provides string constants in multiple types
	template<class StringWanted, typename ElementType>
	class StringConstants{
	public:

		// Public variables //
		static const ElementType DotCharacter;
		static const ElementType UniversalPathSeparator;
		static const ElementType WindowsPathSeparator;
		static const ElementType SpaceCharacter;

		static const ElementType FirstNumber;
		static const ElementType LastNumber;
		static const ElementType Dash;
		static const ElementType PlusSymbol;

	private:
		StringConstants();
		~StringConstants();
	};

	// Define most common StringConstants types //
	typedef StringConstants<wstring, wchar_t> WstringConstants;
	typedef StringConstants<string, char> NarrowStringConstants;

	// ------------------ StringConstants definitions for new types that comply with char ------------------ //
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::DotCharacter = '.';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::UniversalPathSeparator = '/';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::WindowsPathSeparator = '\\';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::SpaceCharacter = ' ';
	
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::FirstNumber = '0';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::LastNumber = '9';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::Dash = '-';
	template<class StringWanted, typename ElementType> const ElementType StringConstants<StringWanted, ElementType>::PlusSymbol = '+';


	//! \brief Singleton class that has string processing functions
	//!
	//! Most functions work with any type of string, but it is recommended to only pass string or wstring to avoid headaches.
	class StringOperations{
	public:
		//! Quick testing function which fails to compile if something is wrong
		DLLEXPORT static bool PerformTesting(const int &tests);

		template<typename CharType>
		DLLEXPORT static bool IsCharacterWhitespace(CharType character){
			if((int)character <= 32)
				return true;
			return false;
		}

		// ------------------ Path related operations ------------------ //
		template<class StringTypeN, typename CharType>
		DLLEXPORT static const StringTypeN RemoveExtension(const StringTypeN &filepath, bool delpath = true){

			size_t startcopy = 0;
			size_t endcopy;

			size_t lastdot = filepath.find_last_of(StringConstants<StringTypeN, CharType>::DotCharacter);

			if(lastdot == StringTypeN::npos){
				// no dot //
				endcopy = filepath.size()-1;
			} else {
				endcopy = lastdot-1;
			}

			// Potentially erase from beginning //
			if(delpath){
				// Find last path character //
				size_t lastpath = 0;

				for(size_t i = 0; i < filepath.size(); i++){
					if(filepath[i] == StringConstants<StringTypeN, CharType>::UniversalPathSeparator || filepath[i] == 
						StringConstants<StringTypeN, CharType>::WindowsPathSeparator)
					{
						// Currently last found path //
						lastpath = i;
					}
				}

				if(lastpath != 0){
					// Set start //
					startcopy = lastpath+1;
				}
			}

			// Return empty if no data is valid //
			if(startcopy > endcopy || startcopy >= filepath.size() || endcopy >= filepath.size())
				return StringTypeN();

			// return the wanted part //
			return filepath.substr(startcopy, endcopy-startcopy+1);
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static const StringTypeN GetExtension(const StringTypeN &filepath){
			size_t startcopy = 0;
			size_t endcopy = filepath.size()-1;

			size_t lastdot = filepath.find_last_of(StringConstants<StringTypeN, CharType>::DotCharacter);

			if(lastdot == StringTypeN::npos){
				// no dot //
				return StringTypeN();
			}

			startcopy = lastdot+1;

			// Return empty if no data is valid //
			if(startcopy > endcopy || startcopy >= filepath.size() || endcopy >= filepath.size())
				return StringTypeN();

			// Return the extension //
			return filepath.substr(startcopy, endcopy-startcopy+1);
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static const StringTypeN ChangeExtension(const StringTypeN& filepath, const StringTypeN &newext){
			size_t startcopy = 0;
			size_t endcopy = filepath.size()-1;

			size_t lastdot = filepath.find_last_of(StringConstants<StringTypeN, CharType>::DotCharacter);

			if(lastdot != StringTypeN::npos){
				// dot found //
				endcopy = lastdot;

			} else {
				// No dot, so just append it //
				return filepath+newext;
			}

			// Return empty if no data is valid //
			if(startcopy > endcopy || startcopy >= filepath.size() || endcopy >= filepath.size())
				return StringTypeN();

			// Return the extension //
			return filepath.substr(startcopy, endcopy-startcopy+1)+newext;
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static const StringTypeN RemovePath(const StringTypeN &filepath){
			size_t startcopy = 0;
			size_t endcopy = filepath.size()-1;

			// Find last path character //
			size_t lastpath = 0;

			for(size_t i = 0; i < filepath.size(); i++){
				if(filepath[i] == StringConstants<StringTypeN, CharType>::UniversalPathSeparator || filepath[i] == 
					StringConstants<StringTypeN, CharType>::WindowsPathSeparator)
				{
					// Currently last found path //
					lastpath = i;
				}
			}

			if(lastpath != 0){
				// Set start //
				startcopy = lastpath+1;
			}

			// Return empty if no data is valid //
			if(startcopy > endcopy || startcopy >= filepath.size() || endcopy >= filepath.size())
				return StringTypeN();
			

			// return the wanted part //
			return filepath.substr(startcopy, endcopy-startcopy+1);
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static const StringTypeN GetPath(const StringTypeN &filepath){
			size_t startcopy = 0;
			size_t endcopy = filepath.size()-1;

			// Find last path character //
			int lastpath = -1;

			for(size_t i = 0; i < filepath.size(); i++){
				if(filepath[i] == StringConstants<StringTypeN, CharType>::UniversalPathSeparator || filepath[i] == 
					StringConstants<StringTypeN, CharType>::WindowsPathSeparator)
				{
					// Currently last found path //
					lastpath = i;
				}
			}

			if(lastpath < 0){
				// Set start //
				return StringTypeN();
			}

			// Set up copy //
			endcopy = lastpath;


			// Return empty if no data is valid //
			if(startcopy > endcopy || startcopy >= filepath.size() || endcopy >= filepath.size())
				return StringTypeN();


			// return the wanted part //
			return filepath.substr(startcopy, endcopy-startcopy+1);
		}


		// ------------------ General string operations ------------------ //
		template<class StringTypeN>
		DLLEXPORT static bool CutString(const StringTypeN &strtocut, const StringTypeN &separator, vector<StringTypeN>& vec){
			// scan the input and gather positions for string copying //
			vector<Int2> CopyOperations;
			bool PositionStarted = false;

			for(size_t i = 0; i < strtocut.length(); i++){
				if(!PositionStarted){
					PositionStarted = true;
					// add new position index //
					CopyOperations.push_back(Int2(i, -1));
				}

				if(strtocut[i] == separator[0]){
					// Found a possible match //
					// test further //
					size_t modifier = 0;
					bool WasMatch = false;
					while(strtocut[i+modifier] == separator[modifier]){
						// check can it increase without going out of bounds //
						if((strtocut.length() > i+modifier+1) && (separator.length() > modifier+1)){
							// increase modifier to move forward //
							modifier++;
						} else {
							// check is it a match
							if(modifier+1 == separator.length()){
								// found match! //

								// end old string to just before this position //
								CopyOperations.back().Y = i; /*-1; not this because we would have to add 1 in the copy phase anyway */

								PositionStarted = false;
								// skip separator //
								WasMatch = true;
								break;
							}
							break;
						}
					}

					// skip the separator amount of characters, if it was found //
					if(WasMatch)
						// -1 here so that first character of next string won't be missing, because of the loop incrementation //
						i += separator.length()-1;
				}
			}

			if(CopyOperations.size() < 2){
				// would be just one string, for legacy (actually we don't want caller to think it got cut) reasons we return nothing //
				return false;
			}

			// make sure final position has end //
			if(CopyOperations.back().Y < 0)
				CopyOperations.back().Y = strtocut.length();
			// length-1 is not used here, because it would have to be added in copy phase to the substring length, and we didn't add that earlier... //

			// make space //
			vec.reserve(CopyOperations.size());

			// loop through positions and copy substrings to result vector //
			for(size_t i = 0; i < CopyOperations.size(); i++){
				// copy using std::wstring method for speed //
				vec.push_back(strtocut.substr((size_t)CopyOperations[i].X, (size_t)(CopyOperations[i].Y-CopyOperations[i].X)));
			}

			// cutting succeeded //
			return true;
		}

		template<class StringTypeN>
		DLLEXPORT static int CountOccuranceInString(const StringTypeN &data, const StringTypeN &lookfor){

			int count = 0;

			for(size_t i = 0; i < data.length(); i++){
				if(data[i] == lookfor[0]){
					// Found a possible match //
					// test further //
					size_t modifier = 0;
					bool WasMatch = false;
					while(data[i+modifier] == lookfor[modifier]){
						// check can it increase without going out of bounds //
						if((data.length() > i+modifier+1) && (lookfor.length() > modifier+1)){
							// increase modifier to move forward //
							modifier++;
						} else {
							// check is it a match
							if(modifier+1 == lookfor.length()){
								// found match! //
								count++;
								WasMatch = true;
								break;
							}
							break;
						}
					}
					// skip the separator amount of characters, if it was found //
					if(WasMatch)
						// -1 here so that first character of next string won't be missing, because of the loop incrementation //
						i += lookfor.length()-1;
				}
			}
			return count;
		}

		template<class StringTypeN>
		DLLEXPORT static StringTypeN Replace(const StringTypeN &data, const StringTypeN &toreplace, const StringTypeN &replacer){
			// We construct an output string from the wanted bits //
			StringTypeN out;

			if(toreplace.size() < 1){
				// Might want to assert here //
				return StringTypeN();
			}

			int copystart = -1;
			int copyend = -1;

			// loop through data and copy final characters to out string //
			for(size_t i = 0; i < data.size(); i++){
				// check for replaced part //
				if(data[i] == toreplace[0]){
					// check for match //
					bool IsMatch = false;
					for(size_t checkind = 0; (checkind < toreplace.size()) && (checkind < data.size()); checkind++){
						if(data[i+checkind] != toreplace[checkind]){
							// didn't match //
							break;
						}
						// check is final iteration //
						if(!((checkind+1 < toreplace.size()) && (checkind+1 < data.size()))){
							// is a match //
							IsMatch = true;
							break;
						}
					}
					if(IsMatch || toreplace.size() == 1){
						// First add proper characters //
						if(copystart > -1 && copyend > -1)
							out += data.substr(copystart, copyend-copystart+1);

						copystart = -1;
						copyend = -1;

						// it is a match, copy everything in replacer and add toreplace length to i //
						out += replacer;

						i += toreplace.length()-1;
						continue;
					}
				}
				// non matching character mark as to copy //
				if(copystart == -1){
					copystart = i;
				} else {
					copyend = i;
				}
			}

			// Copy rest to out //
			if(copystart > -1 && copyend > -1)
				out += data.substr(copystart, copyend-copystart+1);

			// Return finished string //
			return out;
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static StringTypeN RemoveFirstWords(const StringTypeN &data, int amount){

			size_t firstpos = 0;
			// Find the copy start position //
			int spaces = 0;
			int words = 0;

			for(size_t i = 0; i < data.length(); i++){
				if(data[i] == StringConstants<StringTypeN, CharType>::SpaceCharacter){
					spaces++;
					continue;
				}
				if(spaces > 0){
					words++;
					if(words == amount){
						// This is the spot we want to start from //
						firstpos = i;
						break;
					}
					spaces = 0;
				}
			}

			if(firstpos == 0){
				// Didn't remove anything? //
				return StringTypeN();
			}

			// Generate sub string from start to end //
			return data.substr(firstpos, data.size()-firstpos);
		}

		template<class StringTypeN>
		DLLEXPORT static StringTypeN StitchTogether(const vector<StringTypeN*> &data, const StringTypeN &separator){
			StringTypeN ret;
			bool first = true;
			// reserve space //
			int totalcharacters = 0;

			// This might be faster than not reserving space //
			for(size_t i = 0; i < data.size(); i++){
				totalcharacters += data[i]->length();
			}
			totalcharacters += separator.length()*data.size();
			
			// By reserving space we don't have to allocate more memory during copying which might be faster //
			ret.reserve(totalcharacters);

			for(size_t i = 0; i < data.size(); i++){
				if(!first)
					ret += separator;
				ret += *data[i];
				first = false;
			}

			return ret;
		}

		template<class StringTypeN>
		DLLEXPORT static StringTypeN StitchTogether(const vector<shared_ptr<StringTypeN>> &data, const StringTypeN &separator){
			StringTypeN ret;
			bool first = true;
			// reserve space //
			int totalcharacters = 0;

			// This might be faster than not reserving space //
			for(size_t i = 0; i < data.size(); i++){
				totalcharacters += data[i]->length();
			}
			totalcharacters += separator.length()*data.size();

			// By reserving space we don't have to allocate more memory during copying which might be faster //
			ret.reserve(totalcharacters);

			for(size_t i = 0; i < data.size(); i++){
				if(!first)
					ret += separator;
				ret += *data[i].get();
				first = false;
			}

			return ret;
		}

		template<class StringTypeN>
		DLLEXPORT static void WstringRemovePreceedingTrailingSpaces(StringTypeN &str){
			Int2 CutPositions(-1,-1);

			// search the right part of the string //
			for(size_t i = 0; i < str.size(); i++){
				if(!IsCharacterWhitespace(str[i])){
					if(CutPositions[0] == -1){
						// beginning ended //
						CutPositions.X = i;
					} else {
						// set last pos as this //

					}
					continue;
				}
				if(CutPositions[0] == -1){
					// still haven't found a character //
					continue;
				}
				// check is this last character //
				size_t a = str.size()-1;
				bool found = false;
				for(a; a > i; a--){
					if(!IsCharacterWhitespace(str[a])){
						// there is still valid characters //
						found = true;
						break;
					}
				}
				if(found){
					// skip to the found non-space character //
					i = a-1;
					continue;
				}
				// end found //
				CutPositions.Y = i-1;
				break;
			}

			if(CutPositions.X == -1){
				// nothing in the string //
				str.clear();
				return;
			}
			if(CutPositions.Y == -1){
				if(CutPositions.X == -1){
					// just the first character required //
					CutPositions.Y = CutPositions.X;
				} else {
					// no need to cut from the end //
					CutPositions.Y = str.length()-1;
				}
			}

			// set the wstring as it's sub string //
			str = str.substr((size_t)CutPositions[0], (size_t)(CutPositions[1]-CutPositions[0]+1));
		}

		template<class StringTypeN>
		DLLEXPORT static bool CompareInsensitive(const StringTypeN &data, const StringTypeN &second){
			if(data.size() != second.size())
				return false;

			for(unsigned int i = 0; i < data.size(); i++){
				if(!(Convert::ToLower(data[i]) == Convert::ToLower(second[i]))){
					return false;
				}
			}
			return true;
		}

		template<class StringTypeN>
		DLLEXPORT static bool StringStartsWith(const StringTypeN &data, const StringTypeN &tomatch){
			size_t foundstop = data.find(tomatch);
			return foundstop != StringTypeN::npos && foundstop == 0;
		}

		template<class StringTypeN, typename CharType>
		DLLEXPORT static bool IsStringNumeric(const StringTypeN &data){
			for(size_t i = 0; i < data.size(); i++){
				if((data[i] < StringConstants<StringTypeN, CharType>::FirstNumber || data[i] > StringConstants<StringTypeN, CharType>::LastNumber) &&
					data[i] != StringConstants<StringTypeN, CharType>::Dash && data[i] != StringConstants<StringTypeN, CharType>::DotCharacter &&
					data[i] != StringConstants<StringTypeN, CharType>::PlusSymbol)
				{
					return false;
				}
			}
			return true;
		}
		

		// ------------------ Named non-template versions ------------------ //
		DLLEXPORT FORCE_INLINE static const wstring GetExtensionWstring(const wstring &filepath){
			return GetExtension<wstring, wchar_t>(filepath);
		}
		DLLEXPORT FORCE_INLINE static const string GetExtensionString(const string &filepath){
			return GetExtension<string, char>(filepath);
		}

		DLLEXPORT FORCE_INLINE static const wstring GetPathWstring(const wstring &filepath){
			return GetPath<wstring, wchar_t>(filepath);
		}
		DLLEXPORT FORCE_INLINE static const string GetPathString(const string &filepath){
			return GetPath<string, char>(filepath);
		}

		DLLEXPORT FORCE_INLINE static const wstring RemoveExtensionWstring(const wstring &filepath, bool delpath = true){
			return RemoveExtension<wstring, wchar_t>(filepath, delpath);
		}
		DLLEXPORT FORCE_INLINE static const string RemoveExtensionString(const string &filepath, bool delpath = true){
			return RemoveExtension<string, char>(filepath, delpath);
		}

		DLLEXPORT FORCE_INLINE static const wstring ChangeExtensionWstring(const wstring &filepath, const wstring &newext){
			return ChangeExtension<wstring, wchar_t>(filepath, newext);
		}
		DLLEXPORT FORCE_INLINE static const string ChangeExtensionString(const string &filepath, const string &newext){
			return ChangeExtension<string, char>(filepath, newext);
		}

		DLLEXPORT FORCE_INLINE static const wstring RemovePathWstring(const wstring &filepath){
			return RemovePath<wstring, wchar_t>(filepath);
		}
		DLLEXPORT FORCE_INLINE static const string RemovePathString(const string &filepath){
			return RemovePath<string, char>(filepath);
		}


	private:
		StringOperations();
		~StringOperations();
	};



}
#endif
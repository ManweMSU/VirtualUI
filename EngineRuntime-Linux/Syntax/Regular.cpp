#include "Regular.h"

namespace Engine
{
	namespace Syntax
	{
		namespace FilePatternHelper
		{
			bool CheckPatternAt(const string & path, const string & filter, int pfrom, int ffrom)
			{
				if (ffrom >= filter.Length() && pfrom < path.Length()) return false;
				if (ffrom < filter.Length() && pfrom >= path.Length()) {
					for (int k = ffrom; k < filter.Length(); k++) if (filter[k] != L'*') return false;
					if (ffrom == filter.Length() - 1) return true;
				}
				if (ffrom >= filter.Length()) return true;
				int i = ffrom;
				int j = pfrom;
				while (true) {
					int ep = i;
					while (ep < filter.Length() && filter[ep] != L'*' && filter[ep] != L'?' && filter[ep] != L'\\' && filter[ep] != L'/') ep++;
					if (ep > i) {
						if (path.Length() - j < ep - i) return false;
						if (string::CompareIgnoreCase(path.Fragment(j, ep - i), filter.Fragment(i, ep - i))) return false;
						j += ep - i;
						i = ep;
					}
					if (i >= filter.Length()) return j >= path.Length();
					if (j >= path.Length()) return i >= filter.Length() || CheckPatternAt(path, filter, j, i);
					if (filter[i] == L'*') {
						for (int k = j; k <= path.Length(); k++) {
							if (CheckPatternAt(path, filter, k, i + 1)) return true;
						}
						return false;
					} else if (filter[i] == L'?') {
						i++; j++;
					} else if (filter[i] == L'\\') {
						if (path[i] != L'\\' && path[i] != L'/') return false;
						i++; j++;
					} else if (filter[i] == L'/') {
						if (path[i] != L'\\' && path[i] != L'/') return false;
						i++; j++;
					}
				}
			}
		}
		bool MatchFilePattern(const string & path, const string & filter)
		{
			if (filter == L"*") return true;
			auto filters = filter.Split(L';');
			for (int i = 0; i < filters.Length(); i++) if (FilePatternHelper::CheckPatternAt(path, filters[i], 0, 0)) return true;
			return false;
		}
	}
}
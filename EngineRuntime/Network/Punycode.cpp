#include "Punycode.h"

#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace Network
	{
		string UnicodeToPunycode(const string & text)
		{
			bool regular = true;
			for (int i = 0; i < text.Length(); i++) if (text[i] & 0xFFFFFF80) regular = false;
			if (regular) return text;
			Array<uint32> ucs(0x10);
			ucs.SetLength(text.GetEncodedLength(Encoding::UTF32) + 1);
			text.Encode(ucs.GetBuffer(), Encoding::UTF32, true);
			DynamicString result;
			int run_length = 0;
			uint32 max_char = 0;
			for (int i = 0; i < ucs.Length() - 1; i++) {
				if (ucs[i] > max_char) max_char = ucs[i];
				if (ucs[i] < 128) {
					run_length++;
					result += string(ucs.GetBuffer() + i, 1, Encoding::UTF32);
				}
			}
			if (run_length) result += L'-';
			run_length++;
			uint32 cur_char = 128;
			int state = 0;
			int last_state = -1;
			int bias = 72;
			while (cur_char <= max_char) {
				uint32 min_char = max_char;
				for (int i = 0; i < ucs.Length(); i++) if (ucs[i] >= cur_char && ucs[i] < min_char) min_char = ucs[i];
				if (min_char > cur_char) {
					for (int i = 0; i < ucs.Length(); i++) if (ucs[i] < min_char) state += min_char - cur_char;
					cur_char = min_char;
				}
				for (int i = 0; i < ucs.Length(); i++) {
					if (ucs[i] == cur_char) {
						int ds = state - last_state - 1;
						if (ds) {
							int ids = ds;
							int div = 0;
							while (true) {
								div += 36;
								int edge = max(min(div - bias, 26), 1);
								if (ds < edge) break;
								result += L"abcdefghijklmnopqrstuvwxyz0123456789"[edge + ((ds - edge) % (36 - edge))];
								ds = (ds - edge) / (36 - edge);
							}
							result += L"abcdefghijklmnopqrstuvwxyz0123456789"[ds];
							if (last_state == -1) ids /= 700;
							else ids /= 2;
							ids += (ids / run_length);
							int k = 0;
							while (ids > 35 * 13) {
								ids /= 35;
								k += 36;
							}
							bias = k + (36 * ids) / (ids + 38);
						} else {
							result += L'a';
						}
						last_state = state;
						run_length++;
					}
					if (ucs[i] <= cur_char) state++;
				}
				cur_char++;
			}
			return L"xn--" + result;
		}
		string PunycodeToUnicode(const string & text)
		{
			if (text.Length() >= 4 && text.Fragment(0, 4) == L"xn--") {
				int xs = text.FindLast(L'-'), dp;
				Array<uint32> ucs(0x40);
				if (xs >= 4) {
					for (int i = 4; i < xs; i++) ucs << text[i];
					dp = xs + 1;
				} else dp = 4;
				int bias = 72;
				uint32 char_dec = 128;
				int pos_at = 0;
				bool first = true;
				while (dp < text.Length()) {
					int delta = 0;
					int reg = 36;
					int weight = 1;
					while (true) {
						if (dp >= text.Length()) break;
						int digit = 0;
						if (text[dp] >= L'A' && text[dp] <= L'Z') digit = text[dp] - L'A';
						else if (text[dp] >= L'a' && text[dp] <= L'z') digit = text[dp] - L'a';
						else if (text[dp] >= L'0' && text[dp] <= L'9') digit = text[dp] - L'0' + 26;
						delta += digit * weight;
						int edge = max(min(reg - bias, 26), 1);
						dp++;
						if (digit < edge) break;
						weight *= 36 - edge;
						reg += 36;
					}
					int ds = delta;
					if (first) { ds /= 700; first = false; } else ds /= 2;
					ds += (ds / (ucs.Length() + 1));
					int k = 0;
					while (ds > 35 * 13) { ds /= 35; k += 36; }
					bias = k + (36 * ds) / (ds + 38);
					for (int i = 0; i < delta; i++) {
						pos_at++;
						if (pos_at > ucs.Length()) { pos_at = 0; char_dec++; }
					}
					ucs.Insert(char_dec, pos_at);
					pos_at++;
					if (pos_at > ucs.Length()) { pos_at = 0; char_dec++; }
				}
				return string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32);
			} else return text;
		}
		string DomainNameToPunycode(const string & text)
		{
			auto domains = text.Split(L'.');
			DynamicString result;
			for (int i = 0; i < domains.Length(); i++) {
				if (i) result += L'.';
				result += UnicodeToPunycode(domains[i].LowerCase());
			}
			return result;
		}
		string DomainNameToUnicode(const string & text)
		{
			auto domains = text.Split(L'.');
			DynamicString result;
			for (int i = 0; i < domains.Length(); i++) {
				if (i) result += L'.';
				result += PunycodeToUnicode(domains[i]);
			}
			return result;
		}
	}
}
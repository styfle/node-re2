#include "./wrapped_re2.h"
#include "./util.h"

#include <algorithm>
#include <limits>
#include <vector>


using std::min;
using std::numeric_limits;
using std::vector;

using v8::Array;
using v8::Local;
using v8::String;


NAN_METHOD(WrappedRE2::Split) {
	NanScope();

	Local<Array> result = NanNew<Array>();

	// unpack arguments

	WrappedRE2* re2 = ObjectWrap::Unwrap<WrappedRE2>(args.This());
	if (!re2) {
		result->Set(0, args[0]);
		NanReturnValue(result);
	}

	StrVal a(args[0]);
	StringPiece str(a);

	size_t limit = numeric_limits<size_t>::max();
	if (args.Length() > 1 && args[1]->IsNumber()) {
		size_t lim = args[1]->NumberValue();
		if (lim > 0) {
			limit = lim;
		}
	}


	// actual work

	vector<StringPiece> groups(re2->regexp.NumberOfCapturingGroups() + 1), pieces;
	const StringPiece& match = groups[0];
	size_t lastIndex = 0;

	while (lastIndex < a.size && re2->regexp.Match(str, lastIndex, a.size, RE2::UNANCHORED,
			&groups[0], groups.size())) {
		if (match.size()) {
			if (match.data() == a.data || match.data() - a.data > lastIndex) {
				pieces.push_back(StringPiece(a.data + lastIndex, match.data() - a.data - lastIndex));
			}
			lastIndex = match.data() - a.data + match.size();
			pieces.insert(pieces.end(), groups.begin() + 1, groups.end());
		} else {
			size_t sym_size = getUtf8CharSize(a.data[lastIndex]);
			pieces.push_back(StringPiece(a.data + lastIndex, sym_size));
			lastIndex += sym_size;
		}
		if (pieces.size() >= limit) {
			break;
		}
	}
	if (pieces.size() < limit && (lastIndex < a.size || (lastIndex == a.size && match.size()))) {
		pieces.push_back(StringPiece(a.data + lastIndex, a.size - lastIndex));
	}

	if (pieces.empty()) {
		result->Set(0, args[0]);
		NanReturnValue(result);
	}

	// form a result

	if (a.isBuffer) {
		for (size_t i = 0, n = min(pieces.size(), limit); i < n; ++i) {
			const StringPiece& item = pieces[i];
			result->Set(i, NanNewBufferHandle(item.data(), item.size()));
		}
	} else {
		for (size_t i = 0, n = min(pieces.size(), limit); i < n; ++i) {
			const StringPiece& item = pieces[i];
			result->Set(i, NanNew<String>(item.data(), item.size()));
		}
	}

	NanReturnValue(result);
}

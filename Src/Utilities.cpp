#include "../Include/Utilities.h"

using namespace std;

vector <string> splitString(string text, string delimiter) {
	string token = "";
	int pos = 0;
	vector<string> results;

	while ((pos = text.find(delimiter)) != string::npos) {
		token = text.substr(0, pos);
		results.push_back(token);
		text.erase(0, pos + delimiter.length());
	}
	if (text != "")
		results.push_back(text);

	return results;
}

int customRound(double number) {
	int base = floor(number);

	if (number - base >= 0.4f)
		return ceil(number);
	else
		return floor(number);
}

string standarizeSize(string text, int size) {
	int textSize = text.size();
	string result = text;

	for (int i = 0; i < size - textSize; i++) {
		result = "0" + result;
	}

	return result;
}
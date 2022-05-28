#include "json.h"

using namespace std;

namespace json {

	namespace {

		Node LoadNode(istream& input);

		Node LoadArray(istream& input) {
			Array result;
			
			for (char c; input >> c && c != ']';) {
				if (c != ',') {
					input.putback(c);
				}
				result.push_back(LoadNode(input));
			}
			if (result.empty()) {
				throw ParsingError("Array parsing error");
			}
			return Node(move(result));
		}

		Node LoadNumber(istream& input) {
			std::string parsed_num;

			// Считывает в parsed_num очередной символ из input
			auto read_char = [&parsed_num, &input] {
				parsed_num += static_cast<char>(input.get());
				if (!input) {
					throw ParsingError("Failed to read number from stream"s);
				}
			};

			// Считывает одну или более цифр в parsed_num из input
			auto read_digits = [&input, read_char] {
				if (!std::isdigit(input.peek())) {
					throw ParsingError("A digit is expected"s);
				}
				while (std::isdigit(input.peek())) {
					read_char();
				}
			};

			if (input.peek() == '-') {
				read_char();
			}
			// Парсим целую часть числа
			if (input.peek() == '0') {
				read_char();
				// После 0 в JSON не могут идти другие цифры
			}
			else {
				read_digits();
			}

			bool is_int = true;
			// Парсим дробную часть числа
			if (input.peek() == '.') {
				read_char();
				read_digits();
				is_int = false;
			}

			// Парсим экспоненциальную часть числа
			if (int ch = input.peek(); ch == 'e' || ch == 'E') {
				read_char();
				if (ch = input.peek(); ch == '+' || ch == '-') {
					read_char();
				}
				read_digits();
				is_int = false;
			}

			try {
				if (is_int) {
					// Сначала пробуем преобразовать строку в int
					try {
						return Node(std::stoi(parsed_num));
					}
					catch (...) {
						// В случае неудачи, например, при переполнении,
						// код ниже попробует преобразовать строку в double
					}
				}
				return Node(std::stod(parsed_num));
			}
			catch (...) {
				throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
			}
		}

		Node LoadString(istream& input) {
			/*string line;
			getline(input, line, '"');
			return Node(move(line));*/
			auto it = std::istreambuf_iterator<char>(input);
			auto end = std::istreambuf_iterator<char>();
			std::string s;
			while (true) {
				if (it == end) {
					// Поток закончился до того, как встретили закрывающую кавычку?
					throw ParsingError("String parsing error");
				}
				const char ch = *it;
				if (ch == '"') {
					// Встретили закрывающую кавычку
					++it;
					break;
				}
				else if (ch == '\\') {
					// Встретили начало escape-последовательности
					++it;
					if (it == end) {
						// Поток завершился сразу после символа обратной косой черты
						throw ParsingError("String parsing error");
					}
					const char escaped_char = *(it);
					// Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
					switch (escaped_char) {
					case 'n':
						s.push_back('\n');
						break;
					case 't':
						s.push_back('\t');
						break;
					case 'r':
						s.push_back('\r');
						break;
					case '"':
						s.push_back('"');
						break;
					case '\\':
						s.push_back('\\');
						break;
					default:
						// Встретили неизвестную escape-последовательность
						throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
					}
				}
				else if (ch == '\n' || ch == '\r') {
					// Строковый литерал внутри- JSON не может прерываться символами \r или \n
					throw ParsingError("Unexpected end of line"s);
				}
				else {
					// Просто считываем очередной символ и помещаем его в результирующую строку
					s.push_back(ch);
				}
				++it;
			}

			return Node(std::move(s));
		}

		Node LoadDict(istream& input) {
			Dict result;

			for (char c; input >> c && c != '}';) {
				if (c == ',') {
					input >> c;
				}

				string key = LoadString(input).AsString();
				input >> c;
				result.insert({ move(key), LoadNode(input) });
			}
			if (result.empty()) {
				throw ParsingError("Map parsing error");
			}

			return Node(move(result));
		}

		Node LoadNull(istream& input) {
			char c;
			string word=""s;
			while (input >> c && word != "null"s) {
				word += c;
			}
			if (word != "null"s) {
				throw ParsingError("Parsing null error");
			}
			input.putback(c);
			return Node();
		}
		Node LoadTrue(istream& input) {
			char c;
			string word = ""s;
			while (input >> c && word != "true"s) {
				word += c;
			}
			if (word != "true"s) {
				throw ParsingError("Parsing true error");
			}
			input.putback(c);
			return Node(true);
		}
		Node LoadFalse(istream& input) {
			char c;
			string word = ""s;
			while (input >> c && word != "false"s) {
				word += c;
			}
			if (word != "false"s) {
				throw ParsingError("Parsing false error");
			}
			input.putback(c);
			return Node(false);
		}

		Node LoadNode(istream& input) {
			char c;
			input >> c;

			if (c == '[') {
				return LoadArray(input);
			}
			else if (c == '{') {
				return LoadDict(input);
			}
			else if (c == '"') {
				return LoadString(input);
			}
			else if (c == 'n') {
				input.putback(c);
				return LoadNull(input);
			}
			else if (c == 't') {
				input.putback(c);
				return LoadTrue(input);
			}
			else if (c == 'f') {
				input.putback(c);
				return LoadFalse(input);
			}
			else {
				input.putback(c);
				return LoadNumber(input);
			}
		}

	}  // namespace

	Node::Node(std::nullptr_t value)
		: value_(value) {
	}

	Node::Node(int value)
		: value_(value) {
	}

	Node::Node(bool value) 
		: value_(value) {
	}

	Node::Node(double value) 
		: value_(value) {
	}

	Node::Node(string value)
		: value_(std::move(value)) {
	}
	Node::Node(Array array)
		: value_(std::move(array)) {
	}

	Node::Node(Dict map)
		: value_(std::move(map)) {
	}



	bool Node::IsInt() const {
		return holds_alternative<int>(value_);
	}

	bool Node::IsDouble() const {
		return holds_alternative<int>(value_) || holds_alternative<double>(value_);
	}

	bool Node::IsPureDouble() const {
		return holds_alternative<double>(value_);
	}

	bool Node::IsBool() const {
		return holds_alternative<bool>(value_);
	}

	bool Node::IsString() const {
		return holds_alternative<string>(value_);
	}

	bool Node::IsNull() const {
		return holds_alternative<nullptr_t>(value_);
	}

	bool Node::IsArray() const {
		return holds_alternative<Array>(value_);
	}

	bool Node::IsMap() const {
		return holds_alternative<Dict>(value_);
	}

	int Node::AsInt() const {
		if (!IsInt()) {
			throw std::logic_error("logic error");
		}
		return get<int>(value_);
	}

	bool Node::AsBool() const
	{
		if (!IsBool()) {
			throw std::logic_error("logic error");
		}
		return get<bool>(value_);
	}

	double Node::AsDouble() const
	{
		if (!IsDouble()) {
			throw std::logic_error("logic error");
		}
		if (IsInt()) {
			return static_cast<double>(get<int>(value_));
		}
		return get<double>(value_);
	}

	const string& Node::AsString() const {
		if (!IsString()) {
			throw std::logic_error("logic error");
		}
		return get<string>(value_);
	}

	const Array& Node::AsArray() const {
		if (!IsArray()) {
			throw std::logic_error("logic error");
		}
		return get<Array>(value_);
	}

	const Dict& Node::AsMap() const {
		if (!IsMap()) {
			throw std::logic_error("logic error");
		}
		return get<Dict>(value_);
	}

	Document::Document(Node root)
		: root_(move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root_;
	}

	/*bool operator==(Node left, Node right)
	{
		return left.GetValue() == right.GetValue();
	}

	bool operator!=(Node left, Node right)
	{
		return !(left == right);
	}*/

	Document Load(istream& input) {
		return Document{ LoadNode(input) };
	}

	void Print(const Document& doc, std::ostream& output) {
		/*(void)&doc;
		(void)&output;*/
		PrintNode(doc.GetRoot(), output);
		// Реализуйте функцию самостоятельно
	}

}  // namespace json
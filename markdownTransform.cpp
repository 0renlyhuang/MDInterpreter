#include "markdownTransform.h"
#include <algorithm>
#include <string>


MarkdownTransform::MarkdownTransform(const std::string&& filename) {
	Croot = new Cnode("");
	root = new Node(nul);
	now = root;

	std::ifstream fin(filename);

	bool newPara = false,
		inBlock = false;

	while (getline(fin, s)) {

		// 处理不在代码块中且需要换行
		if (!inBlock && isCutline(s)) {
			now = root;
			now->ch.push_back(new Node(hr));
			newPara = false;
			continue;
		}

		pair<int, string> ps = start(s);

		// 处理不在代码块且没有统计到空格和Tab
		if (!inBlock && ps.second.empty()) {
			now = root;
			newPara = true;
			continue;
		}

		// 去除掉文本行中的空格和Tab之后分析类型
		pair<int, string> tj = judgeType(ps.second);

		// 如果是代码类型
		if (tj.first == blockcode) {
			inBlock ? now->ch.push_back(new Node(nul)) : now->ch.push_back(new Node(blockcode));
			inBlock = !inBlock;
			continue;
		}

		if (inBlock) {
			now->ch.back()->elem[0] += s + '\n';
			continue;
		}

		// 如果判断结果的类型是普通段落
		if (tj.first == paragraph) {
			// 如果当前节点是root
			if (now == root) {
				now = findNode(ps.first);
				now->ch.push_back(new Node(paragraph));
				now = now->ch.back();
			}

			// 如果当前是新段落并且当前节点的节点列表是非空的
			if (newPara && !now->ch.empty()) {
				Node *ptr = nullptr;
				for (auto i : now->ch)
					if (i->type == nul)
						ptr = i;

				// 如果ptr指向一个类型为nul的节点
				if (ptr != nullptr)
					mkpara(ptr);

				now->ch.push_back(new Node(paragraph));
				now = now->ch.back();
			}

			now->ch.push_back(new Node(nul));
			insert(now->ch.back(), tj.second);
			newPara = false;
			continue;
		}

		now = findNode(ps.first);

		// 如果是标题行，修改节点标签，插入正文节点和目录节点
		if (tj.first >= h1 && tj.first <= h6) {
			now->ch.push_back(new Node(tj.first));
			now->ch.back()->elem[0] = "tag" + std::to_string(++cntTag);
			insert(now->ch.back(), tj.second);
			Cins(Croot, tj.first - h1 + 1, tj.second, cntTag);
		}

		// 如果是无序列表
		if (tj.first == ul) {
			if (now->ch.empty() || now->ch.back()->type != ul) {
				now->ch.push_back(new Node(ul));
			}
			now = now->ch.back();

			bool flag = false;
			if (newPara && !now->ch.empty()) {
				Node *ptr = nullptr;
				for (auto i : now->ch)
					if (i->type == li)
						ptr = i;

				// 最后一个类型为li的节点
				if (ptr != nullptr)
					mkpara(ptr);
				flag = true;
			}

			now->ch.push_back(new Node(li));
			now = now->ch.back();

			if (flag) {
				now->ch.push_back(new Node(paragraph));
				now = now->ch.back();
			}

			insert(now, tj.second);
		}

		// 如果是有序列表
		if (tj.first == ol) {
			if (now->ch.empty() || now->ch.back()->type != ol) {
				now->ch.push_back(new Node(ol));
			}
			now = now->ch.back();

			bool flag = false;
			if (newPara && !now->ch.empty()) {
				Node *ptr = nullptr;
				for (auto i : now->ch)
					if (i->type == li)
						ptr = i;

				if (ptr != nullptr)
					mkpara(ptr);
				flag = true;
			}
			now->ch.push_back(new Node(li));
			now = now->ch.back();

			if (flag) {
				now->ch.push_back(new Node(paragraph));
				now = now->ch.back();
			}
			insert(now, tj.second);
		}

		// 如果是引用
		if (tj.first == quote) {
			if (now->ch.empty() || now->ch.back()->type != quote) {
				now->ch.push_back(new Node(quote));
			}
			now = now->ch.back();
			if (newPara || now->ch.empty())
				now->ch.push_back(new Node(paragraph));

			insert(now->ch.back(), tj.second);
		}

		newPara = false;
	}
	fin.close();

	dfs(root);

	_table += "<ul>";
	for (int i = 0; i < static_cast<int>(Croot->ch.size()); ++i)
		Cdfs(Croot->ch.at(i), std::to_string(i + 1) + ".");
	_table += "</ul>";
}


// 目录插入新节点 
void MarkdownTransform::Cins(Cnode *v, int x, const string &hd, int tag) {
	if (x == 1) {
		v->ch.push_back(new Cnode(hd));
		v->ch.back()->tag = "tag" + std::to_string(tag);
		return;
	}
	if (v->ch.empty() || v->ch.back()->heading.empty())
		v->ch.push_back(new Cnode(""));
	Cins(v->ch.back(), x - 1, hd, tag);
}


// 正文插入新节点 逐个字符解析
void MarkdownTransform::insert(Node *v, const string &src) {
	bool incode = false,
		inem = false,
		instrong = false,
		inautolink = false;
	v->ch.push_back(new Node(nul));

	for (int i = 0; i < static_cast<int>(src.size()); ++i) {
		char ch = src.at(i);
		int n = src.size();

		// 如果是'\'（表示转义）
		if (ch == '\\' && i + 1 < n) {
			v->ch.back()->elem[0] += string(1, src.at(++i));
			continue;
		}

		// 如果是`（表示一行代码），第一次匹配设置为代码，第二次匹配设置为正文
		if (ch == '`' && !inautolink) { // autolink???
			incode ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(code));
			incode = !incode;
			continue;
		}

		// 如果是*且下一个字符是*（表示加粗）
		if (ch == '*' && (i + 1 < n) && (src.at(i + 1) == '*') &&
			!incode && !inautolink) {
			++i;
			instrong ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(strong));
			instrong = !instrong;
			continue;
		}

		// 如果是_（表示斜体）
		if (ch == '_' && !incode && !instrong && !inautolink) {
			inem ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(em));
			inem = !inem;
			continue;
		}

		// 如果是!且下一个字符是[（表示图片）      ![]()
		if (ch == '!' && (i + 4 < n) && (src.at(i + 1) == '[') &&
			!incode && !instrong && !inem && !inautolink) {
			//v->ch.push_back(new Node(image));
			int reset = i + 1; //指向[
			string text("");
			// 跳过!和[
			for (i += 2; i + 2 < n && src.at(i) != ']'; ++i)
				text.append(string(1, src.at(i)));

			// 如果至少还有两个字符并且下一个字符是(，则当前字符是]，
			if (i + 2 < n && src.at(i + 1) == '(') {

				//// 如果还有下一个字符且当前字符是(
				//if (i + 1 < n && src.at(i) == '(') {
				string url("");
				// 跳过],(, 初始字符是(的下一个
				for (i += 2; (i < n) && (src.at(i) != ' ') && (src.at(i) != ')'); ++i)
					url.append(string(1, src.at(i)));

				// 跳过所有的空格
				while (i < n && src.at(i) == ' ')
					++i;

				// 如果至少还有一个字符，且当前字符是"
				if (i + i < n && src.at(i) == '"') {
					string title = "";
					for (++i; i < n && src.at(i) != '"'; ++i) {
						title.append(string(1, src.at(i)));
					}
					// 匹配到了第二个"和)
					if (src.at(i) == '"' && i + 1 < n && src.at(i + 1) == ')') {
						++i;
						v->ch.push_back(new Node(image));
						v->ch.back()->elem = { text, url, title };
						continue;
					}
					// 没有匹配到第二个",或者匹配到了第二个"但没有匹配到)
					else {
						i = reset;
						v->ch.push_back(new Node(nul)); // !
						v->ch.push_back(new Node(nul)); // [
						continue;
					}
				}
				// 如果匹配到没有title的![]()
				else if (i < n && src.at(i) == ')') {
					v->ch.push_back(new Node(image));
					v->ch.back()->elem = { text, url, "" };
					continue;
				}
				// i > n，不匹配
				else {
					i = reset;
					v->ch.push_back(new Node(nul)); // !
					v->ch.push_back(new Node(nul)); // [
					continue;
				}
			}
			// 如果不是 至少还有两个字符并且下一个字符是(
			else {
				i = reset;
				v->ch.push_back(new Node(nul)); // !
				v->ch.push_back(new Node(nul)); // [
				continue;
			}
		}

		// 如果是[(表示链接）  []() TODO(hjf): 匹配失败的结果统一返回
		if (src.at(i) == '[' && i + 3 < n &&
			!incode && !instrong && !inem && !inautolink) {
			int reset = i; // 指向[
			string text = "";
			// 跳过[
			for (++i; i + 2 < n && src.at(i) != ']'; ++i)
				text.append(string(1, src.at(i)));

			if ((src.at(i) == ']') && (i + 2 < n) && (src.at(i + 1) == '(')) {
				string url = "";
				// 跳到(后一个字符
				for (i += 2; (i < n) && (src.at(i) != ')') && (src.at(i) != ' '); ++i)
					url.append(string(1, src.at(i)));

				if (i >= n) {
					i = reset;
					v->ch.push_back(new Node(nul)); // [
					continue;
				}

				// 跳过所有空格
				while (i < n && src.at(i) == ' ')
					++i;

				//匹配到没有title的[]()
				if (i < n && src.at(i) == ')') {
					v->ch.push_back(new Node(href));
					v->ch.back()->elem = { text, url, "" };
					continue;
				}
				// 匹配到第一个"
				else if (i < n && src.at(i) == '"') {
					string title = "";
					// 跳过第一个"
					for (++i; i + 1 < n && src.at(i) != '"'; ++i)
						title.append(string(1, src.at(i)));

					// 匹配到第二个"和)
					if (i + 1 < n && src.at(i) == '"' && src.at(i + 1) == ')') {
						++i;
						v->ch.push_back(new Node(href));
						v->ch.back()->elem = { text, url, title };
						continue;
					}

					// 字符数不够（")）或没有匹配到"或匹配到"但下一个字符不是)
					else {
						i = reset;
						v->ch.push_back(new Node(nul)); // [
						continue;
					}
				}
				// 空格之后既不是“也不是）
				else {
					i = reset;
					v->ch.push_back(new Node(nul)); // [
					continue;
				}
			}
			// 没有匹配到]或剩余字符数不超过2（还有()未匹配）或匹配到]但下一个字符不是(
			else {
				i = reset;
				v->ch.push_back(new Node(nul));
				continue;
			}
		}
		// 对普通字符的处理
		v->ch.back()->elem[0] += string(1, ch);
		if (inautolink)
			v->ch.back()->elem[1] += string(1, ch);
	}

	// 如果行末的是两个空格的情况（表示br）
	if ((src.size() >= 2) && (src.at(src.size() - 1) == ' ') && (src.at(src.size() - 2) == ' '))
		v->ch.push_back(new Node(br));
}

// 判断是否换行
bool MarkdownTransform::isCutline(const string &src) {
	int cnt = 0;
	for (char ch : src) {
		if (ch != ' ' && ch != '\t' && ch != '-')
			return false;
		if (ch == '-')
			++cnt;
	}
	return (cnt >= 3);
}

// 生成段落
void MarkdownTransform::mkpara(Node *v) {
	if (v->ch.size() == 1u && v->ch.back()->type == paragraph)
		return;
	if (v->type == paragraph)
		return;
	if (v->type == nul) {
		v->type = paragraph;
		return;
	}
	Node *x = new Node(paragraph);
	x->ch = v->ch;
	v->ch.clear();
	v->ch.push_back(x);
}

// 遍历正文节点 生成HTML字符串
void MarkdownTransform::dfs(Node *v) {
	if (v->type == paragraph && v->elem[0].empty() && v->ch.empty())
		return;

	// 元素开始标签
	_content += frontTag.at(v->type);

	if (isHeadding(v))
		_content += "id=\"" + v->elem[0] + "\">";
	else if (isHref(v))
		_content += "<a href=\"" + v->elem[1] + "\"title = \"" + v->elem[2] +
		"\">" + v->elem[0] + "</a>";
	else if (isImage(v))
		_content += "<img alt=\"" + v->elem[0] + "\" src=\"" + v->elem[1] +
		"\" title=\"" + v->elem[2] + "\" />";
	else
		_content += v->elem[0];

	// 递归遍历所有子节点
	for (Node *n : v->ch)
		dfs(n);

	// 元素结束标签
	_content += backTag[v->type];
}

// 遍历目录节点，生成HTML字符串
void MarkdownTransform::Cdfs(Cnode *v, string index) {
	_table += "<li>\n"; // 有序列表
						// v->tag的格式是"tag" + std::to_string(tag)
	_table += "<a href=\"#" + v->tag + "\">" + index + " " + v->heading + "</a>\n";

	int n = v->ch.size();
	if (n) {
		_table += "<ul>\n"; // 无序列表
		for (int i = 0; i < n; ++i)
			Cdfs(v->ch.at(i), index + std::to_string(i + 1) + ".");
		_table += "</ul>\n";
	}
	_table += "</li>\n";
}
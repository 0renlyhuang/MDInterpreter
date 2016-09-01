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

		// �����ڴ����������Ҫ����
		if (!inBlock && isCutline(s)) {
			now = root;
			now->ch.push_back(new Node(hr));
			newPara = false;
			continue;
		}

		pair<int, string> ps = start(s);

		// �����ڴ������û��ͳ�Ƶ��ո��Tab
		if (!inBlock && ps.second.empty()) {
			now = root;
			newPara = true;
			continue;
		}

		// ȥ�����ı����еĿո��Tab֮���������
		pair<int, string> tj = judgeType(ps.second);

		// ����Ǵ�������
		if (tj.first == blockcode) {
			inBlock ? now->ch.push_back(new Node(nul)) : now->ch.push_back(new Node(blockcode));
			inBlock = !inBlock;
			continue;
		}

		if (inBlock) {
			now->ch.back()->elem[0] += s + '\n';
			continue;
		}

		// ����жϽ������������ͨ����
		if (tj.first == paragraph) {
			// �����ǰ�ڵ���root
			if (now == root) {
				now = findNode(ps.first);
				now->ch.push_back(new Node(paragraph));
				now = now->ch.back();
			}

			// �����ǰ���¶��䲢�ҵ�ǰ�ڵ�Ľڵ��б��Ƿǿյ�
			if (newPara && !now->ch.empty()) {
				Node *ptr = nullptr;
				for (auto i : now->ch)
					if (i->type == nul)
						ptr = i;

				// ���ptrָ��һ������Ϊnul�Ľڵ�
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

		// ����Ǳ����У��޸Ľڵ��ǩ���������Ľڵ��Ŀ¼�ڵ�
		if (tj.first >= h1 && tj.first <= h6) {
			now->ch.push_back(new Node(tj.first));
			now->ch.back()->elem[0] = "tag" + std::to_string(++cntTag);
			insert(now->ch.back(), tj.second);
			Cins(Croot, tj.first - h1 + 1, tj.second, cntTag);
		}

		// ����������б�
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

				// ���һ������Ϊli�Ľڵ�
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

		// ����������б�
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

		// ���������
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


// Ŀ¼�����½ڵ� 
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


// ���Ĳ����½ڵ� ����ַ�����
void MarkdownTransform::insert(Node *v, const string &src) {
	bool incode = false,
		inem = false,
		instrong = false,
		inautolink = false;
	v->ch.push_back(new Node(nul));

	for (int i = 0; i < static_cast<int>(src.size()); ++i) {
		char ch = src.at(i);
		int n = src.size();

		// �����'\'����ʾת�壩
		if (ch == '\\' && i + 1 < n) {
			v->ch.back()->elem[0] += string(1, src.at(++i));
			continue;
		}

		// �����`����ʾһ�д��룩����һ��ƥ������Ϊ���룬�ڶ���ƥ������Ϊ����
		if (ch == '`' && !inautolink) { // autolink???
			incode ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(code));
			incode = !incode;
			continue;
		}

		// �����*����һ���ַ���*����ʾ�Ӵ֣�
		if (ch == '*' && (i + 1 < n) && (src.at(i + 1) == '*') &&
			!incode && !inautolink) {
			++i;
			instrong ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(strong));
			instrong = !instrong;
			continue;
		}

		// �����_����ʾб�壩
		if (ch == '_' && !incode && !instrong && !inautolink) {
			inem ? v->ch.push_back(new Node(nul)) : v->ch.push_back(new Node(em));
			inem = !inem;
			continue;
		}

		// �����!����һ���ַ���[����ʾͼƬ��      ![]()
		if (ch == '!' && (i + 4 < n) && (src.at(i + 1) == '[') &&
			!incode && !instrong && !inem && !inautolink) {
			//v->ch.push_back(new Node(image));
			int reset = i + 1; //ָ��[
			string text("");
			// ����!��[
			for (i += 2; i + 2 < n && src.at(i) != ']'; ++i)
				text.append(string(1, src.at(i)));

			// ������ٻ��������ַ�������һ���ַ���(����ǰ�ַ���]��
			if (i + 2 < n && src.at(i + 1) == '(') {

				//// ���������һ���ַ��ҵ�ǰ�ַ���(
				//if (i + 1 < n && src.at(i) == '(') {
				string url("");
				// ����],(, ��ʼ�ַ���(����һ��
				for (i += 2; (i < n) && (src.at(i) != ' ') && (src.at(i) != ')'); ++i)
					url.append(string(1, src.at(i)));

				// �������еĿո�
				while (i < n && src.at(i) == ' ')
					++i;

				// ������ٻ���һ���ַ����ҵ�ǰ�ַ���"
				if (i + i < n && src.at(i) == '"') {
					string title = "";
					for (++i; i < n && src.at(i) != '"'; ++i) {
						title.append(string(1, src.at(i)));
					}
					// ƥ�䵽�˵ڶ���"��)
					if (src.at(i) == '"' && i + 1 < n && src.at(i + 1) == ')') {
						++i;
						v->ch.push_back(new Node(image));
						v->ch.back()->elem = { text, url, title };
						continue;
					}
					// û��ƥ�䵽�ڶ���",����ƥ�䵽�˵ڶ���"��û��ƥ�䵽)
					else {
						i = reset;
						v->ch.push_back(new Node(nul)); // !
						v->ch.push_back(new Node(nul)); // [
						continue;
					}
				}
				// ���ƥ�䵽û��title��![]()
				else if (i < n && src.at(i) == ')') {
					v->ch.push_back(new Node(image));
					v->ch.back()->elem = { text, url, "" };
					continue;
				}
				// i > n����ƥ��
				else {
					i = reset;
					v->ch.push_back(new Node(nul)); // !
					v->ch.push_back(new Node(nul)); // [
					continue;
				}
			}
			// ������� ���ٻ��������ַ�������һ���ַ���(
			else {
				i = reset;
				v->ch.push_back(new Node(nul)); // !
				v->ch.push_back(new Node(nul)); // [
				continue;
			}
		}

		// �����[(��ʾ���ӣ�  []() TODO(hjf): ƥ��ʧ�ܵĽ��ͳһ����
		if (src.at(i) == '[' && i + 3 < n &&
			!incode && !instrong && !inem && !inautolink) {
			int reset = i; // ָ��[
			string text = "";
			// ����[
			for (++i; i + 2 < n && src.at(i) != ']'; ++i)
				text.append(string(1, src.at(i)));

			if ((src.at(i) == ']') && (i + 2 < n) && (src.at(i + 1) == '(')) {
				string url = "";
				// ����(��һ���ַ�
				for (i += 2; (i < n) && (src.at(i) != ')') && (src.at(i) != ' '); ++i)
					url.append(string(1, src.at(i)));

				if (i >= n) {
					i = reset;
					v->ch.push_back(new Node(nul)); // [
					continue;
				}

				// �������пո�
				while (i < n && src.at(i) == ' ')
					++i;

				//ƥ�䵽û��title��[]()
				if (i < n && src.at(i) == ')') {
					v->ch.push_back(new Node(href));
					v->ch.back()->elem = { text, url, "" };
					continue;
				}
				// ƥ�䵽��һ��"
				else if (i < n && src.at(i) == '"') {
					string title = "";
					// ������һ��"
					for (++i; i + 1 < n && src.at(i) != '"'; ++i)
						title.append(string(1, src.at(i)));

					// ƥ�䵽�ڶ���"��)
					if (i + 1 < n && src.at(i) == '"' && src.at(i + 1) == ')') {
						++i;
						v->ch.push_back(new Node(href));
						v->ch.back()->elem = { text, url, title };
						continue;
					}

					// �ַ���������")����û��ƥ�䵽"��ƥ�䵽"����һ���ַ�����)
					else {
						i = reset;
						v->ch.push_back(new Node(nul)); // [
						continue;
					}
				}
				// �ո�֮��Ȳ��ǡ�Ҳ���ǣ�
				else {
					i = reset;
					v->ch.push_back(new Node(nul)); // [
					continue;
				}
			}
			// û��ƥ�䵽]��ʣ���ַ���������2������()δƥ�䣩��ƥ�䵽]����һ���ַ�����(
			else {
				i = reset;
				v->ch.push_back(new Node(nul));
				continue;
			}
		}
		// ����ͨ�ַ��Ĵ���
		v->ch.back()->elem[0] += string(1, ch);
		if (inautolink)
			v->ch.back()->elem[1] += string(1, ch);
	}

	// �����ĩ���������ո���������ʾbr��
	if ((src.size() >= 2) && (src.at(src.size() - 1) == ' ') && (src.at(src.size() - 2) == ' '))
		v->ch.push_back(new Node(br));
}

// �ж��Ƿ���
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

// ���ɶ���
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

// �������Ľڵ� ����HTML�ַ���
void MarkdownTransform::dfs(Node *v) {
	if (v->type == paragraph && v->elem[0].empty() && v->ch.empty())
		return;

	// Ԫ�ؿ�ʼ��ǩ
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

	// �ݹ���������ӽڵ�
	for (Node *n : v->ch)
		dfs(n);

	// Ԫ�ؽ�����ǩ
	_content += backTag[v->type];
}

// ����Ŀ¼�ڵ㣬����HTML�ַ���
void MarkdownTransform::Cdfs(Cnode *v, string index) {
	_table += "<li>\n"; // �����б�
						// v->tag�ĸ�ʽ��"tag" + std::to_string(tag)
	_table += "<a href=\"#" + v->tag + "\">" + index + " " + v->heading + "</a>\n";

	int n = v->ch.size();
	if (n) {
		_table += "<ul>\n"; // �����б�
		for (int i = 0; i < n; ++i)
			Cdfs(v->ch.at(i), index + std::to_string(i + 1) + ".");
		_table += "</ul>\n";
	}
	_table += "</li>\n";
}
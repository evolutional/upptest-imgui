#pragma once
/*
upptest-imgui - A simple 'dear imgui'-based test runner ui for upptest.

Author - Oli Wilkinson (https://github.com/evolutional/upptest-imgui)

This file provides both the interface and the implementation.

USAGE
-----

See README.md for details


LICENSE
-------

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "upptest/upptest.h"

#include <algorithm>

namespace utest
{
	namespace imgui
	{
		struct node_stats
		{
			node_stats() : count(0), failed_count(0), pass_count(0) {}

			node_stats(const node_stats& o)
			{
				count = o.count;
				failed_count = o.failed_count;
				pass_count = o.pass_count;
			}

			node_stats(node_stats&& o)
			{
				std::swap(count, o.count);
				std::swap(failed_count, o.failed_count);
				std::swap(pass_count, o.pass_count);
			}

			node_stats& operator = (const node_stats& o)
			{
				count = o.count;
				failed_count = o.failed_count;
				pass_count = o.pass_count;
				return *this;
			}

			node_stats& operator += (const node_stats& o)
			{
				count += o.count;
				failed_count += o.failed_count;
				pass_count += o.pass_count;
				return *this;
			}

			node_stats& operator = (node_stats&& o)
			{
				std::swap(count, o.count);
				std::swap(failed_count, o.failed_count);
				std::swap(pass_count, o.pass_count);
				return *this;
			}

			size_t count;
			size_t failed_count;
			size_t pass_count;
		};

		class test_node
		{
		public:
			typedef std::shared_ptr<test_node> node_ptr;
			typedef std::weak_ptr<test_node> weak_node_ptr;

			test_node()
				: test_node("", nullptr)
			{}

			test_node(const std::string& n, const test_node* p)
				: test_node(n, nullptr, p)
			{}

			test_node(const std::string& n, const info* i, const test_node* p)
				: parent(p), children(), stats()
				, name(n)
				, test_info(i), last_result()
				, selected(false)
			{}

			node_ptr add_child(const std::string& n)
			{
				auto node = std::make_shared<test_node>(n, this);
				children.push_back(node);
				return node;
			}

			node_ptr add_child(const info* i)
			{
				auto node = std::make_shared<test_node>(i->name, i, this);
				children.push_back(node);
				return node;
			}

			node_stats update_stats()
			{
				node_stats n;
				if (test_info)
				{
					n.count = 1;
					n.failed_count = last_result.status == status::fail ? 1 : 0;
					n.pass_count = last_result.status == status::pass ? 1 : 0;
					return n;
				}

				for (const auto& itr : children)
				{
					n += itr->update_stats();
				}
				stats = n;
				return n;
			}

			void update_selected(bool new_selected, bool recurse = true)
			{
				selected = new_selected;
				if (recurse)
				{
					for (const auto& itr : children)
					{
						itr->update_selected(new_selected, recurse);
					}
				}
			}

			const test_node* parent;
			std::vector<node_ptr> children;
			node_stats stats;
			const std::string name;
			const info* test_info;
			result last_result;
			bool selected;
		};

		template<class category_part_iterator>
		inline std::shared_ptr<test_node> add_node(const std::shared_ptr<test_node>& root, const info* i, category_part_iterator cat_itr, const category_part_iterator& cat_itr_end)
		{
			std::shared_ptr<test_node> node;
			if (cat_itr == cat_itr_end)
			{
				return root->add_child(i);
			}

			auto citr = std::find_if(root->children.begin(), root->children.end(), [&](const auto& i) { return i->name == *cat_itr; });
			if (citr == root->children.end())
			{
				node = root->add_child(*cat_itr);
			}
			else
			{
				node = *citr;
			}
			return add_node(node, i, ++cat_itr, cat_itr_end);
		}

		template<class test_container_iterator>
		inline std::shared_ptr<test_node> create_tree(const test_container_iterator& begin, const test_container_iterator& end, const char category_separator = '.')
		{
			auto root = std::make_shared<test_node>();
			for (auto itr = begin; itr != end; ++itr)
			{
				const auto* i = *itr;
				std::stringstream fullname(i->category);
				std::vector<std::string> elems;
				std::string item;
				while (std::getline(fullname, item, category_separator)) {
					elems.push_back(item);
				}
				add_node(root, i, elems.begin(), elems.end());
			}

			root->update_stats();		
			return root;
		}

		template<class test_container>
		inline std::shared_ptr<test_node> create_tree(const test_container& tests)
		{
			return create_tree(tests.begin(), tests.end());
		}

		inline void run_selected(const std::shared_ptr<test_node>& node, bool ignore_selected = false)
		{
			if (!node->test_info)
			{
				for (const auto& child : node->children)
				{
					run_selected(child, ignore_selected);
				}
			}
			else if (ignore_selected || node->selected)
			{
				result res;
				auto r = utest::runner::run(node->test_info, res);
				node->last_result = res;
			}
		}
	
		inline void update_tree(const std::shared_ptr<test_node>& node)
		{
			static const ::ImColor fail_color(200, 50, 50, 255);
			static const ::ImColor pass_color(50, 200, 50, 255);
			static const ::ImColor unk_color(100, 100, 100, 255);

			for (auto i = 0; i < node->children.size(); ++i)
			{
				const auto& child = node->children[i];

				if (child->test_info)
				{
					::ImGui::Selectable(child->name.c_str(), &child->selected);

					ImGui::PushID(child.get());
					if (ImGui::BeginPopupContextItem("test"))
					{
						if (ImGui::Selectable("Run"))
						{
							run_selected(child, true);
							child->update_stats();
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();
					::ImGui::SameLine(300);
					if (child->last_result.status == status::not_run)
					{
						::ImGui::TextColored(unk_color, "Not run");
					}
					else if (child->last_result.status == status::fail)
					{
						::ImGui::TextColored(fail_color, "Failed: %s", child->last_result.err_message.c_str());
					}
					else
					{
						::ImGui::TextColored(pass_color, "Success");
					}
				}
				else
				{
					auto is_open = ::ImGui::TreeNode(child.get(), "%s (%d tests)", child->name.c_str(), child->stats.count);

					ImGui::PushID(child.get());
					if (ImGui::BeginPopupContextItem("select"))
					{
						if (ImGui::Selectable("Run all"))
						{
							run_selected(child, true);
							child->update_stats();
						}						
						if (ImGui::Selectable("Select all"))
						{
							child->update_selected(true);
						}
						if (ImGui::Selectable("Select none"))
						{
							child->update_selected(false);
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();

					if (child->stats.failed_count)
					{
						::ImGui::SameLine(300); ::ImGui::TextColored(fail_color, "Failed: %d test(s) failed", child->stats.failed_count);
					}
					else if (child->stats.pass_count == child->stats.count)
					{
						::ImGui::SameLine(300); ::ImGui::TextColored(pass_color, "Success");
					}
					else
					{
						::ImGui::SameLine(300); ::ImGui::TextColored(unk_color, "Not run");
					}
					if (is_open)
					{
						update_tree(child);
						::ImGui::TreePop();
					}
				}
			}
		}
				
		inline void window(const std::shared_ptr<test_node>& node, float w = 550, float h = 680)
		{
			::ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiSetCond_FirstUseEver);
			bool open(true);
			if (::ImGui::Begin("utest", &open))
			{
				if (::ImGui::Button("Run All"))
				{
					run_selected(node, true);
					node->update_stats();
				}
				ImGui::SameLine();
				if (::ImGui::Button("Run selected"))
				{
					run_selected(node);
					node->update_stats();
				}
				update_tree(node);
				::ImGui::End();
			}
		}
	}
}
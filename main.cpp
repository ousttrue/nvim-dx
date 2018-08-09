#include <process.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <msgpackpp.h>
#include <vector>
#include <functional>


class Dispatcher
{
	std::vector<uint8_t> m_buffer;
	std::unordered_map<std::string, msgpackpp::procedurecall> m_method_map;

public:
	Dispatcher()
	{
	}

	void push_bytes(const char *bytes, size_t n)
	{
		auto pos = m_buffer.size();
		m_buffer.resize(pos + n);
		for (int i = 0; i < n; ++i, ++pos)
		{
			m_buffer[pos] = bytes[i];
		}

		while (!m_buffer.empty()) {
			if (!consume()) {
				break;
			}
		}
	}

	void add_method(const std::string &method_name, const msgpackpp::procedurecall &proc)
	{
		m_method_map.insert(std::make_pair(method_name, proc));
	}

private:
	void redraw(const msgpackpp::parser &args)
	{
		if (args.is_array()) {
			std::cout << "[redraw]";
			auto child = args[0];
			for (int i = 0; i < args.count(); ++i) {
				
				auto cmd = child[0].get_string();

				auto found = m_method_map.find(cmd);
				if (found != m_method_map.end()) {
					// found
					auto proc = found->second;

					try {
						proc(child[1]);
					}
					catch (const std::exception &ex)
					{
						std::cout << ex.what() << " " << child[1] << std::endl;
					}
				}
				else {
					std::cout << ", unknown " << cmd;
				}

				child = child.next();
			}
			std::cout << std::endl;
		}
		else {

			std::cout << "redraw" << std::endl;

		}
	}

	bool consume()
	{
		auto msg = msgpackpp::parser(m_buffer.data(), m_buffer.size());
		auto d = msg.consumed_size();
		std::cout << "msg " << d << " bytes" << std::endl;
		m_buffer.erase(m_buffer.begin(), m_buffer.begin() + d);

		if (!msg.is_array()) {
			throw std::exception("is not error");
		}

		auto msgType = msg[0].get_number<int>();
		if (msgType == 0) {
			// request
			throw std::exception("not implemented");
		}
		else if (msgType == 1) {
			// response
			auto msgId = msg[1].get_number<int>();
			auto error = msg[2];
			auto payload = msg[3];

			std::cout
				<< "response: " << msgId
				//<< " => " << payload 
				<< std::endl;
		}
		else if (msgType == 2) {
			// notify
			auto method = msg[1].get_string();

			if (method == "redraw") {

				redraw(msg[2]);

			}
			else {

				std::cout
					<< "notify: " << method
					//<<  " => " << payload 
					<< std::endl;
			}

		}
		else {
			throw std::exception("invali msgType");
		}
	}
};


struct Highlight
{
	bool bold = false;
	int foreground = 255;
};
MPPP_MAP_SERIALIZER(Highlight, 2, bold, foreground);


///
/// https://github.com/neovim/neovim/blob/master/runtime/doc/ui.txt
///
class Grid
{
public:
	void option_set(std::string key, bool enable) {

	}

	void default_colors_set(int i0, int i1, int i2, int i3, int i4) {

	}

	void update_fg(int g) {
	}

	void update_bg(int g) {
	}

	void update_sp(int g) {
	}

	void resize(int cols, int rows) {

	}

	void clear() {

	}

	void cursor_goto(int col, int row) {

	}

	void highlight_set(Highlight hl) {

	}

	void put(std::string str)
	{

	}

	void mode_info_set(bool cursor_style_enabled, msgpackpp::parser map)
	{
		//std::cout << "mode_info_set: " << map << std::endl;
	}

	void mode_change(std::string mode, int value)
	{

	}
};


int main(int argc, char **argv)
{
	std::cout << std::endl << "Example 5 - demonstrates Process::try_get_exit_status" << std::endl;

	Dispatcher dispatcher;
	Grid grid;

#define DISPATCHER_ADD_METHOD(method) dispatcher.add_method(#method, msgpackpp::make_methodcall(&grid, &Grid::method))
	DISPATCHER_ADD_METHOD(option_set);
	DISPATCHER_ADD_METHOD(default_colors_set);
	DISPATCHER_ADD_METHOD(update_fg);
	DISPATCHER_ADD_METHOD(update_bg);
	DISPATCHER_ADD_METHOD(update_sp);
	DISPATCHER_ADD_METHOD(resize);
	DISPATCHER_ADD_METHOD(clear);
	DISPATCHER_ADD_METHOD(cursor_goto);
	DISPATCHER_ADD_METHOD(highlight_set);
	DISPATCHER_ADD_METHOD(put);
	DISPATCHER_ADD_METHOD(mode_info_set);
	DISPATCHER_ADD_METHOD(mode_change);
#undef DISPACHER_ADD_METHOD

	TinyProcessLib::Process process1("nvim --embed", "", [&dispatcher](const char *bytes, size_t n) {

		dispatcher.push_bytes(bytes, n);

	}, nullptr, true);

	msgpackpp::packer packer;
	packer.pack_array(4);
	packer.pack_integer(0);
	packer.pack_integer(1);
	packer.pack_str("nvim_ui_attach");
	packer.pack_array(3);
	packer.pack_integer(80);
	packer.pack_integer(56);
	packer.pack_map(0);

	// std::vector<std::uint8_t>
	auto p = packer.get_payload();
	process1.write((const char*)p.data(), p.size());

	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto exit_status = process1.get_exit_status();

	return exit_status;
}

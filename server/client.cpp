#include "server.hpp"

#include <boost/bind.hpp>

#include <regex>

client::pointer client::create(boost::asio::io_service &io, server &serv) {
	return pointer(new client(io, serv));
}

void client::start() {
	boost::asio::async_read(this->socket, boost::asio::buffer(this->buf, this->buf.size()), wrap_function(&client::handle_registration));
}

void client::write(std::string message) {
}

client::client(boost::asio::io_service &io, server &serv) : socket(io), io(io), serv(serv) {
}

void client::read_message(std::function<void(client*, std::string)> func, const boost::system::error_code &err, size_t size) {
	if(err) {
		shutdown();
		return;
	}

	std::string message;
	std::copy(this->buf.begin(), this->buf.begin() + size, std::back_inserter(message));
	func(this, message);
}

std::function<void(const boost::system::error_code&, size_t)> client::wrap_function(std::function<void(client*, std::string)> func) {
	return boost::bind(&client::read_message, this, func, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
}

void client::handle_registration(std::string message) {
	std::regex registration_pattern("^Hi. I'm a ([^ ]*)$");
	std::smatch match;
	auto matches = std::regex_match(message, match, registration_pattern);

	if(matches && match.size() == 2) {
		this->type = match[1];
		auto clients = serv.clients[type];
		clients.insert(shared_from_this());
	}
	else
		shutdown();

	listen();
}

void client::listen() {
	boost::asio::async_read(this->socket, boost::asio::buffer(this->buf, this->buf.size()), wrap_function(&client::handle_read));
}

void client::handle_read(std::string message) {
}

void client::handle_write(const boost::system::error_code &err) {
	if(err)
		this->shutdown();
}

void client::shutdown() {
	this->socket.shutdown(tcp::socket::shutdown_both);
	this->socket.close();
	serv.clients[this->type].erase(shared_from_this());
}

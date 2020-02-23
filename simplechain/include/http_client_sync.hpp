#ifndef HTTP_CLIENT_SYNC_HPP
#define HTTP_CLIENT_SYNC_HPP


namespace simplechain {

	class HTTP_CLIENT {
	public:
		static int post(const std::string& host, const std::string& port, const std::string& page, const std::string& data, std::string& reponse_data);
	};
}

#endif
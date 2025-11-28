/*
 * KNP module 2 Sample starter code using Restinio library (HTTP with C++) for Client-Server
 * and you can also use this in your PRJ3 project
 * Underviser: "Jenny" Jung Min Kim
 *
 * Purpose:
 *   Minimal example showing how to use the Restinio library to handle HTTP protocol and
 *   HTTP-based Client–Server communication. 
 *   (You’ll learn HTTP and implementations in detail in November in the KNP course.)
 *
 * IMPORTANT — Adapt for your PRJ3 project:
 *   ( You will learn these steps in KNP module 2 gradually)
 *   1) Replace book_t/book_collection/book_collection_t/book_handler_t
 *      with your own data structure, collections, and handlers. 
 *   2) Update the initial values in main() to match your data and logic.
 *   3) Add the HTTP endpoints you need (GET/POST/PUT/DELETE, etc.).
 */
#include <string>
#include <iostream>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>
#include "mqtt_subscriber.hpp"
#include <thread>

//=========================*/
// Example data structure
// TODO later: Replace 'book_t' with your own struct (e.g. something_t).
//=========================*/
struct order_t  // This is Data structure (sample code has book_t, you need to define your own data struct)
{
	order_t() = default;

	order_t(std::string station,std::string modtager, std::string genstand)
		: m_station { std::move(station)},m_modtager{ std::move(modtager) }, m_genstand{ std::move(genstand) }
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("station", m_station)
			& json_dto::mandatory("modtager", m_modtager)
			& json_dto::mandatory("genstand", m_genstand);
	}

	std::string m_station;
	std::string m_modtager;
	std::string m_genstand;
};



struct AGV_status_t {
    std::string m_status;

    template <typename JSON_IO>
    void json_io(JSON_IO & io)
    {
        io & json_dto::mandatory("status", m_status);
    }
};

//=========================*/
// Todo later: Change this alias for your project type, e.g. vector<something_t>
//=========================*/
using order_collection_t = std::vector< order_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;
using namespace std::chrono;


//=========================*/
// HTTP handler class
// TODO later: Rename/modify methods for your data model
//=========================*/



// Page handler
class page_handler_t
{
private:
	/* data */
public:

	auto on_homepage(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		
        	return req->create_response()
            .append_header(restinio::http_field::content_type, "text/html")
            .set_body(restinio::sendfile("/home/au774177/projekt3/prj3-bil/server/restinio-bundle/src/index.html"))
            .done();
	}

	auto on_service(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		
        	return req->create_response()
            .append_header(restinio::http_field::content_type, "text/html")
            .set_body(restinio::sendfile("/home/au774177/projekt3/prj3-bil/server/restinio-bundle/src/index.html"))
            .done();
	
	}
	auto on_personal(const restinio::request_handle_t& req, rr::route_params_t) const
	{
        	return req->create_response()
            .append_header(restinio::http_field::content_type, "text/html")
            .set_body(restinio::sendfile("/home/au774177/projekt3/prj3-bil/server/restinio-bundle/src/personaleInterface.html"))
            .done();
	
	}
	
	auto on_get_status(const restinio::request_handle_t& req, rr::route_params_t) {

		std::unique_lock<std::mutex> lock(status_string_mut);


    	AGV_status_t response_struct {AGV_status}; // Laver en instans af en AGV status struct, der understøtter Json formattering. Som parameter gives den delte status string

    	std::string body = json_dto::to_json(response_struct); //Her konverteres der til json

		auto resp = req->create_response();
		resp.append_header("Content-Type", "application/json; charset=utf-8");
		resp.set_body(std::move(body));
		return resp.done();
		}


};



class order_handler_t
{
public:
	explicit order_handler_t(order_collection_t & orders)
		: m_orders(orders)
	{}

	auto on_order_log(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		auto resp = init_resp(req->create_response());

		resp.set_body("Order log (Orders: " +
			std::to_string(m_orders.size()) + ")\n");

		for(std::size_t i = 0; i < m_orders.size(); ++i)
		{
			resp.append_body(std::to_string(i + 1) + ". ");
			const auto & b = m_orders[i];
			resp.append_body("Station: " + b.m_station + ", Modtager: " + b.m_modtager + ", Genstand: " + b.m_genstand + "\n");


		}

		return resp.done();
	}



	auto on_new_order(const restinio::request_handle_t& req, rr::route_params_t) const {

		auto resp = init_resp(req->create_response());
		try
		{
			m_orders.emplace_back(json_dto::from_json<order_t>(req->body())); 
			
		}
		catch(const std::exception& e)
		{
			mark_as_bad_request(resp);
		}
		return resp.done();

	}
	

private:
	order_collection_t & m_orders;  // TODO: Replace 'book_collection_t', m_books with your own ..

	template < typename RESP >
	static RESP init_resp(RESP resp)
	{
		resp
			.append_header("Server", "RESTinio sample server /v.0.6")
			.append_header_date_field()
			.append_header("Content-Type", "text/plain; charset=utf-8");
		return resp;
	}

	static void mark_as_bad_request(auto & resp)  // (auto & resp) with C++20 , if C++17, it should be (RESP & resp)
	{
		resp.header().status_line(restinio::status_bad_request());
	}
};

//=========================*/
// Router setup
// TODO later: Add new endpoints for your project (HTTP POST, PUT, etc.) - we will learn gradually in lecture KNP module2
//=========================*/
auto server_handler(order_collection_t & order_collection) //replace book_ .. 
{
	auto router = std::make_unique<router_t>();
	auto handler_page = std::make_shared<page_handler_t> ();
	auto handler = std::make_shared<order_handler_t> (std::ref(order_collection));


	auto by = [&](auto method) {
		using namespace std::placeholders;
		return std::bind(method, handler, _1, _2);
	};

	auto by_page = [&](auto method) {
		using namespace std::placeholders;
		return std::bind(method, handler_page, _1, _2);
	};

	// Example: GET /
	router->http_get("/ordre", by(&order_handler_t::on_order_log)); 
	router->http_get("/", by_page(&page_handler_t::on_homepage));  
	router->http_get("/personale", by_page(&page_handler_t::on_personal));
	router->http_get("/personale/AGV_status", by_page(&page_handler_t::on_get_status));  
	router->http_post("/submit_personale", by(&order_handler_t::on_new_order));  



	return router;
}




auto start_rest_server(){
	try
	{
		using traits_t = restinio::traits_t<
			restinio::asio_timer_manager_t,
			restinio::single_threaded_ostream_logger_t,
			router_t >;

		//=========================*/
		// TODO later: Replace this hardcoded collection with your own initial values
		//=========================*/
		order_collection_t ordercollection;

		//=========================*/
		// Run server
		// IMPORTANT:
		//   - "0.0.0.0" means server listens on ALL network interfaces.
		//   - Use Pi's IP + port 8080 to connect from another device.
		//=========================*/
		restinio::run(
			restinio::on_this_thread<traits_t>()
				.address("0.0.0.0")   // For Pi: allow access from outside
				.port(8080)           // Default port, change if needed
				.request_handler(server_handler(ordercollection))
				.read_next_http_message_timelimit(10s)
				.write_http_response_timelimit(1s)
				.handle_request_timeout(1s)
		);
		
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

}

//=========================*/
// Main entry point
//=========================*/
int main()
{

	std::thread mqtt_subscriber_thread (mqtt_subscriber);
	std::thread msg_processing_thread (process_status_messages);
	start_rest_server();


	mqtt_subscriber_thread.join();
	msg_processing_thread.join();



	return 0;
}

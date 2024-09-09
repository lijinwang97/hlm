#include "hlm_design_patterns.h"

#include "hlm_logger.h"

crow::response LogWrapper(const crow::request& req, std::function<crow::response(const crow::request&)> handler) {
    hlm_info("Received request url:{}, Body:{}", req.url, req.body);
    crow::response res = handler(req);
    hlm_info("Response url:{}, Body:{}", req.url, res.body);
    return res;
}
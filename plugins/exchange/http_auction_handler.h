/* http_auction_handler.h                                          -*- C++ -*-
   Jeremy Barnes, 14 December 2012
   Copyright (c) 2012 Datacratic.  All rights reserved.

   Class to deal with http connections to an exchange.
*/

#pragma once

#include "soa/service/http_endpoint.h"
#include "soa/service/stats_events.h"
#include "rtbkit/common/auction.h"

namespace RTBKIT {

struct HttpExchangeConnector;


/*****************************************************************************/
/* HTTP AUCTION HANDLER                                                      */
/*****************************************************************************/

struct HttpAuctionHandler
    : public HttpConnectionHandler,
      public std::enable_shared_from_this<HttpAuctionHandler> {

    HttpAuctionHandler();
    ~HttpAuctionHandler();

    /** We got our transport. */
    void onGotTransport();

    HttpExchangeConnector * endpoint;

    std::shared_ptr<Auction> auction;
    bool hasTimer;
    bool disconnected;
    bool servingRequest;  ///< Are we currently, actively serving a request?

    virtual void handleHttpPayload(const HttpHeader & header,
                                   const std::string & payload);

    /** Got a disconnection */
    virtual void handleDisconnect();

    void cancelTimer();

    /** Send over an error response.  Mostly used when the bid request had
        an error.
    */
    virtual void sendErrorResponse(const std::string & error,
                                   const std::string & details = "");
    
    /** Drop the auction.  Mostly when there simply isn't enough time to
        do anything useful. */
    virtual void dropAuction(const std::string & reason = "");

    virtual void handleTimeout(Date date, size_t cookie);

    virtual void onDisassociate();
    virtual void onCleanup();
    virtual std::string status() const;

    /** Record an event that will be stored in the operational events
        table.
    */
    void doEvent(const char * eventName,
                 EventType type = ET_COUNT,
                 float value = 1.0,
                 const char * units = "");

    void incNumServingRequest();
    
    /** Function called once the auction is finished.  It causes the
        response in the auction to be formatted and sent back to the
        exchange.
    */
    virtual void sendResponse();

    /** Return a stringified JSON of the response for our auction.  Default
        implementation calls getResponse() and stringifies the result.

        This version is provided as it may be more efficient in terms of
        memory allocations.

        Default implementation forwards to the endpoint.
    */
    virtual HttpResponse getResponse() const;

    /** Parse the given payload into a bid request.

        Default implementation forwards to the endpoint.
    */
    virtual std::shared_ptr<BidRequest>
    parseBidRequest(const HttpHeader & header,
                    const std::string & payload);

    /** Return the available time for the bid request in milliseconds.  This
        method should not parse the bid request, as when shedding load
        we want to do as little work as possible.

        Most exchanges include this information in the HTTP headers.

        Default implementation forwards to the endpoint.
    */
    virtual double
    getTimeAvailableMs(const HttpHeader & header,
                       const std::string & payload);

    /** Return an estimate of how long a round trip with the connected
        server takes, in milliseconds at the exchange's latency percentile.

        Default implementation forwards to the endpoint.
    */
    virtual double
    getRoundTripTimeMs(const HttpHeader & header);

    static long created;
    static long destroyed;
};


} // namespace RTBKIT

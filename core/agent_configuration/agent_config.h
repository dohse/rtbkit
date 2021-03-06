/* rtb_agent_config.h                                             -*- C++ -*-
   Jeremy Barnes, 24 March 2011
   Configuration for an RTB agent.
*/

#ifndef __rtb_agent_config_h__
#define __rtb_agent_config_h__

#include <string>
#include <vector>
#include <set>
#include "soa/jsoncpp/json.h"
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include "rtbkit/common/bid_request.h"
#include "include_exclude.h"
#include "rtbkit/common/account_key.h"

namespace RTBKIT {

struct BiddableSpots;
struct AgentStats;


/*****************************************************************************/
/* CREATIVE                                                                  */
/*****************************************************************************/

/** Describes a creative that a agent has available. */

struct Creative {
    Creative(int width = 0, int height = 0,
             std::string name = "", int id = -1, int tagId = -1);

    // Three samples that can be used for testing...
    static const Creative sampleLB;
    static const Creative sampleBB;
    static const Creative sampleWS;

    void fromJson(const Json::Value & val);
    Json::Value toJson() const;

    Format format;

    /// To bid with v0.3
    int tagId;

    /// Purely for information
    std::string name;
    int id;

    IncludeExclude<std::string> languageFilter;
    IncludeExclude<CachedRegex<boost::u32regex, Utf8String> > locationFilter;
    IncludeExclude<std::string> exchangeFilter;

    /** Is the given ad spot compatible with the given creative format? */
    bool compatible(const AdSpot & spot) const;

    /** Is this creative biddable on the given exchange and protocol version? */
    bool biddable(const std::string & exchange,
                  const std::string & protocolVersion) const;
};


/*****************************************************************************/
/* USER PARTITION                                                            */
/*****************************************************************************/

/** Describes a way in which users are partitioned consistently in order to
    allow for A/B testing.
*/

struct UserPartition {
    UserPartition();

    void swap(UserPartition & other);

    void clear();

    enum HashOn {
        NONE,        ///< Hash always returns zero
        RANDOM,      ///< Random number

        EXCHANGEID,  ///< Hash on md5(exchange ID)
        PROVIDERID   ///< Hash on md5(provider ID)
    } hashOn;

    int modulus;     ///< Max value of hash that's achievable

    struct Interval {
        Interval(int first = 0, int last = 0)
            : first(first), last(last)
        {
        }

        int first, last;

        bool in(int val) const
        {
            return val >= first && val < last;
        }

        Json::Value toJson() const;
    };

    bool empty() const
    {
        return hashOn == NONE && modulus == 1 && includeRanges.size() == 1
            && includeRanges[0].first == 0 && includeRanges[0].last == 1;
    }

    /** A list of the hash ranges that are accepted. */
    std::vector<Interval> includeRanges;

    /** Return true if the user matches the user partition. */
    bool matches(const UserIds & ids) const;

    /** Parse from JSON. */
    void fromJson(const Json::Value & json);

    /** Return as JSON */
    Json::Value toJson() const;
};


/*****************************************************************************/
/* BLACKLIST CONTROL                                                         */
/*****************************************************************************/

enum BlacklistType {
    BL_OFF,       ///< Don't blacklist
    BL_USER,      ///< Blacklist the user
    BL_USER_SITE  ///< Blacklist the user on the given site
};

enum BlacklistScope {
    BL_AGENT,      ///< Blacklist for the agent
    BL_ACCOUNT,    ///< Blacklist for an account
};


/*****************************************************************************/
/* BID CONTROL TYPE                                                          */
/*****************************************************************************/

enum BidControlType {
    BC_RELAY,       ///< Relay to agent which will compute the price
    BC_RELAY_FIXED, ///< Relay to agent but bid fixed price
    BC_FIXED        ///< Bid fixed price and don't relay
};


/*****************************************************************************/
/* BID RESULT FORMAT                                                         */
/*****************************************************************************/

enum BidResultFormat {
    BRF_FULL,         ///< Full message
    BRF_LIGHTWEIGHT,  ///< Lightweight message
    BRF_NONE          ///< No message
};

Json::Value toJson(BidResultFormat fmt);
void fromJson(BidResultFormat & fmt, const Json::Value & j);

/*****************************************************************************/
/* AGENT CONFIG                                                              */
/*****************************************************************************/

/** Describes the configuration state of an RTB agent.  Passed through by
    a agent to the router to describe how the routes should be set up.
*/
struct AgentConfig {
    AgentConfig();

    static AgentConfig createFromJson(const Json::Value & json);

    void parse(const std::string & jsonStr);
    void fromJson(const Json::Value & json);

    Json::Value toJson(bool includeCreatives = true) const;

    AccountKey account;   ///< Who to bill this to

    bool test;            ///< Can't make real bids

    std::string roundRobinGroup;
    int roundRobinWeight;

    float bidProbability;
    float minTimeAvailableMs;

    int maxInFlight;

    std::vector<std::string> requiredIds;

    IncludeExclude<DomainMatcher> hostFilter;
    IncludeExclude<CachedRegex<boost::regex, std::string> > urlFilter;
    IncludeExclude<CachedRegex<boost::regex, std::string> > languageFilter;
    IncludeExclude<CachedRegex<boost::u32regex, Utf8String> > locationFilter;

    struct SegmentInfo {
        SegmentInfo()
            : excludeIfNotPresent(false)
        {
        }

        bool excludeIfNotPresent;
        SegmentList include;
        SegmentList exclude;

        /** What exchanges is this filter applied to?  If the exchange
            is excluded by the filter, then the filter is bypassed. */
        IncludeExclude<std::string> applyToExchanges;
        
        IncludeExcludeResult process(const SegmentList & segments) const;
        
        void fromJson(const Json::Value & val);
        Json::Value toJson() const;
    };

    std::map<std::string, SegmentInfo> segments;

    IncludeExclude<std::string> exchangeFilter;

    IncludeExclude<AdSpot::Position> foldPositionFilter;

    SegmentInfo tagFilter;

    struct HourOfWeekFilter {

        HourOfWeekFilter();

        bool isIncluded(double auctionDate) const;

        bool isDefault() const;  // true if all hours are 1

        void fromJson(const Json::Value & val);
        Json::Value toJson() const;

    private:
        std::bitset<168> hourBitmap;
    };
    
    HourOfWeekFilter hourOfWeekFilter;

    UserPartition userPartition;

    std::vector<Creative> creatives;

    BlacklistType blacklistType;
    BlacklistScope blacklistScope;
    double blacklistTime;
    
    bool hasBlacklist() const
    {
        return blacklistType != BL_OFF && blacklistTime > 0.0;
    }

    BidControlType bidControlType;
    uint32_t fixedBidCpmInMicros;

    IncludeExclude<std::string> augmentationFilter;

    struct AugmentationInfo {
        std::string name;
        Json::Value config;

        bool operator < (const AugmentationInfo & other) const
        {
            return name < other.name;
        }
    };

    /** Add the given augmentation to the list of augmentations.  This will
        fail if the given augmentation already exists in the list.
    */
    void addAugmentation(const std::string & name,
                         Json::Value config = Json::Value());

    /** Add the given augmentation to the list of augmentations.  This will
        fail if the given augmentation already exists in the list.
    */
    void addAugmentation(AugmentationInfo info);

    std::vector<AugmentationInfo> augmentations;

    /** JSON value that is passed through with each bid. */
    Json::Value providerConfig;

    /** List of channels for which we subscribe to post impression
        visit events.
    */
    SegmentList visitChannels;

    /** Do we include visits not matched to a conversion? */
    bool includeUnmatchedVisits;

    /** Message formats */
    BidResultFormat winFormat, lossFormat, errorFormat;

    /** Returns a list of (adspot, [creatives]) pairs compatible with this
        agent.
    */
    BiddableSpots
    canBid(const std::vector<AdSpot> & spots,
           const std::string & exchange,
           const std::string & protocolVersion,
           const std::string & language,
           const Utf8String & location, uint64_t locationHash,
           ML::Lightweight_Hash<uint64_t, int> & locationCache) const;


    /** Cache used to speed up successive calls to isBiddableRequest() for a
        given request.
    */
    struct RequestFilterCache
    {
        RequestFilterCache(const BidRequest& request) :
            urlHash(hashString(request.url.c_str())),

            language(!request.language.empty() ?
                    request.language : "unspecified"),
            languageHash(hashString(request.language)),

            location(request.location.fullLocationString()),
            locationHash(hashString(location))
        {}

        uint64_t urlHash;

        std::string language;
        uint64_t languageHash;

        Utf8String location;
        uint64_t locationHash;

        // Cache of regex -> bool
        ML::Lightweight_Hash<uint64_t, int> urlFilter;
        ML::Lightweight_Hash<uint64_t, int> languageFilter;
        ML::Lightweight_Hash<uint64_t, int> locationFilter;
    };

    typedef std::function<void(const char*)> FilterStatFn;

    /** Returns the biddable spots (see canBid) if the agent can bid on the
        given bid request.

        Before the function returns false, doFilterStat will be called with the
        cause of the filtering and the appropriate member of AgentStats will be
        incremented.
    */
    BiddableSpots
    isBiddableRequest(
            const BidRequest& request,
            AgentStats& stats,
            RequestFilterCache& cache,
            const FilterStatFn & doFilterStat = FilterStatFn()) const;
};


} // namespace RTBKIT

#endif /* __rtb_agent_config_h__ */

#pragma once
/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <maxscale/cppdefs.hh>
#include <maxscale/filter.h>

namespace maxscale
{

/**
 * @class FilterSession filter.hh <maxscale/filter.hh>
 *
 * FilterSession is a base class for filter sessions. A concrete filter session
 * class should be derived from this class and override all relevant functions.
 *
 * Note that even though this class is intended to be derived from, no functions
 * are virtual. That is by design, as the class will be used in a context where
 * the concrete class is known. That is, there is no need for the virtual mechanism.
 */
class FilterSession
{
public:
    /**
     * @class Downstream
     *
     * An instance of this class represents a component following a filter.
     */
    class Downstream
    {
    public:
        Downstream()
        {
            m_data.instance = NULL;
            m_data.session = NULL;
            m_data.routeQuery = NULL;
        }

        Downstream(const DOWNSTREAM& down)
            : m_data(down)
        {}

        /**
         * Function for sending a packet from the client to the next component
         * in the routing chain towards the backend.
         *
         * @param pPacket  A packet to be delivered towards the backend.
         *
         * @return Whatever the following component returns.
         */
        int routeQuery(GWBUF* pPacket)
        {
            return m_data.routeQuery(m_data.instance, m_data.session, pPacket);
        }

        DOWNSTREAM m_data;
    };

    class Upstream
    {
    public:
        /**
         * @class Upstream
         *
         * An instance of this class represents a component preceeding a filter.
         */
        Upstream()
        {
            m_data.instance = NULL;
            m_data.session = NULL;
            m_data.clientReply = NULL;
        }

        Upstream(const UPSTREAM& up)
            : m_data(up)
        {}

        /**
         * Function for sending a packet from the backend to the next component
         * in the routing chain towards the client.
         *
         * @param pPacket  A packet to be delivered towards the backend.
         *
         * @return Whatever the following component returns.
         */
        int clientReply(GWBUF* pPacket)
        {
            return m_data.clientReply(m_data.instance, m_data.session, pPacket);
        }

        UPSTREAM m_data;
    };

    /**
     * The FilterSession instance will be deleted when a client session
     * has terminated. Will be called only after @c close() has been called.
     */
    ~FilterSession();

    /**
     * Called when a client session has been closed.
     */
    void close();

    /**
     * Called for setting the component following this filter session.
     *
     * @param down The component following this filter.
     */
    void setDownstream(const Downstream& down);

    /**
     * Called for setting the component preceeding this filter session.
     *
     * @param up The component preceeding this filter.
     */
    void setUpstream(const Upstream& up);

    /**
     * Called when a packet being is routed to the backend. The filter should
     * forward the packet to the downstream component.
     *
     * @param pPacket A client packet.
     */
    int routeQuery(GWBUF* pPacket);

    /**
     * Called when a packet is routed to the client. The filter should
     * forward the packet to the upstream component.
     *
     * @param pPacket A client packet.
     */
    int clientReply(GWBUF* pPacket);

    /**
     * Called for obtaining diagnostics about the filter session.
     *
     * @param pDcb  The dcb where the diagnostics should be written.
     */
    void diagnostics(DCB *pDcb);

protected:
    FilterSession(SESSION* pSession);

protected:
    SESSION*   m_pSession; /*< The SESSION this filter session is associated with. */
    Downstream m_down;     /*< The downstream component. */
    Upstream   m_up;       /*< The upstream component. */
};


/**
 * @class Filter filter.hh <maxscale/filter.hh>
 *
 * An instantiation of the Filter template is used for creating a filter.
 * Filter is an example of the "Curiously recurring template pattern"
 * https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 * that is used for compile time polymorfism.
 *
 * The typical way for using the template is as follows:
 *
 * @code
 * class MyFilterSession : public maxscale::FilterSession
 * {
 *     // Override the relevant functions.
 * };
 *
 * class MyFilter : public maxscale::Filter<MyFilter, MyFilterSession>
 * {
 * public:
 *      static MyFilter* create(const char* zName, char** pzOptions, FILTER_PARAMETER** ppParams);
 *
 *      MyFilterSession* newSession(SESSION* pSession);
 *
 *      static uint64_t getCapabilities();
 * };
 * @endcode
 *
 * The concrete filter class must implement the methods @c create, @c newSession and
 * @c getCapabilities, with the prototypes as shown above.
 *
 * The plugin function @c GetModuleObject is then implemented as follows:
 *
 * @code
 * extern "C" FILTER_OBJECT *GetModuleObject()
 * {
 *     return &MyFilter::s_object;
 * };
 * @endcode
 */
template<class FilterType, class FilterSessionType>
class Filter
{
public:
    static FILTER* createInstance(const char* zName, char** pzOptions, FILTER_PARAMETER** ppParams)
    {
        FilterType* pFilter = NULL;

        MXS_EXCEPTION_GUARD(pFilter = FilterType::create(zName, pzOptions, ppParams));

        return reinterpret_cast<FILTER*>(pFilter);
    }

    static void* newSession(FILTER* pInstance, SESSION* pSession)
    {
        FilterType* pFilter = reinterpret_cast<FilterType*>(pInstance);
        void* pFilterSession;

        MXS_EXCEPTION_GUARD(pFilterSession = pFilter->newSession(pSession));

        return pFilterSession;
    }

    static void closeSession(FILTER*, void* pData)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        MXS_EXCEPTION_GUARD(pFilterSession->close());
    }

    static void freeSession(FILTER*, void* pData)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        MXS_EXCEPTION_GUARD(delete pFilterSession);
    }

    static void setDownstream(FILTER*, void* pData, DOWNSTREAM* pDownstream)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        typename FilterSessionType::Downstream down(*pDownstream);

        MXS_EXCEPTION_GUARD(pFilterSession->setDownstream(down));
    }

    static void setUpstream(FILTER* pInstance, void* pData, UPSTREAM* pUpstream)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        typename FilterSessionType::Upstream up(*pUpstream);

        MXS_EXCEPTION_GUARD(pFilterSession->setUpstream(up));
    }

    static int routeQuery(FILTER* pInstance, void* pData, GWBUF* pPacket)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        int rv = 0;
        MXS_EXCEPTION_GUARD(rv = pFilterSession->routeQuery(pPacket));

        return rv;
    }

    static int clientReply(FILTER* pInstance, void* pData, GWBUF* pPacket)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        int rv = 0;
        MXS_EXCEPTION_GUARD(rv = pFilterSession->clientReply(pPacket));

        return rv;
    }

    static void diagnostics(FILTER* pInstance, void* pData, DCB* pDcb)
    {
        FilterSessionType* pFilterSession = reinterpret_cast<FilterSessionType*>(pData);

        MXS_EXCEPTION_GUARD(pFilterSession->diagnostics(pDcb));
    }

    static uint64_t getCapabilities(void)
    {
        uint64_t rv = 0;

        MXS_EXCEPTION_GUARD(rv = FilterType::getCapabilities());

        return rv;
    }

    static void destroyInstance(FILTER* pInstance)
    {
        FilterType* pFilter = reinterpret_cast<FilterType*>(pInstance);

        MXS_EXCEPTION_GUARD(delete pFilter);
    }

    static FILTER_OBJECT s_object;
};


template<class FilterType, class FilterSessionType>
FILTER_OBJECT Filter<FilterType, FilterSessionType>::s_object =
{
    &Filter<FilterType, FilterSessionType>::createInstance,
    &Filter<FilterType, FilterSessionType>::newSession,
    &Filter<FilterType, FilterSessionType>::closeSession,
    &Filter<FilterType, FilterSessionType>::freeSession,
    &Filter<FilterType, FilterSessionType>::setDownstream,
    &Filter<FilterType, FilterSessionType>::setUpstream,
    &Filter<FilterType, FilterSessionType>::routeQuery,
    &Filter<FilterType, FilterSessionType>::clientReply,
    &Filter<FilterType, FilterSessionType>::diagnostics,
    &Filter<FilterType, FilterSessionType>::getCapabilities,
    &Filter<FilterType, FilterSessionType>::destroyInstance,
};


}
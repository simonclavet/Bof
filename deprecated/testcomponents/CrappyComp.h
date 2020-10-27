#pragma once




#include "TestGoodComponents.h"



using namespace std;

class Server : public GoodSerializable
{
public:
    string m_address = "";
    int m_port = 0;

    GOOD_SERIALIZABLE(
        Server, GOOD_VERSION(1)
        , GOOD(m_address)
        , GOOD(m_port)
    );
};

class ServerList : public GoodSerializable
{
public:
    std::vector<Server> m_servers;
    int m_hoho = 0;

    GOOD_SERIALIZABLE(
        ServerList, GOOD_VERSION(1)
        , GOOD(m_servers)
        , GOOD(m_hoho)
    );
};

class CrappyComp : public GoodSerializable
{
public:
    float m_thing = 0.0f;
    int m_otherThing = 0;
    ServerList m_serverList;

    GOOD_SERIALIZABLE(
        CrappyComp, GOOD_VERSION(1)
        , GOOD(m_thing)
        , GOOD(m_otherThing)
        , GOOD(m_serverList)
    );
};


class SomeStupidSystemUsingCrappy
{
public:
    static void DoStuff(ComponentGrid& grid);
};

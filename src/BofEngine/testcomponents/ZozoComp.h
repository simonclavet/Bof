#pragma once




#include "components/GoodComponents.h"



using namespace std;


class WhackBud : public GoodSerializable
{
public:
    string m_zaza = "";
    int m_crapounet = 0;

    GOOD_SERIALIZABLE(
        WhackBud, 0
        , GOOD(m_zaza)
        , GOOD(m_crapounet)
    );

};

class ZozoComp : public GoodSerializable
{
public:
    vector<int> m_someIntVector{};
    WhackBud m_someWhackBud;

    GOOD_SERIALIZABLE(
        ZozoComp, GOOD_VERSION(1)
        , GOOD(m_someIntVector)
        , GOOD(m_someWhackBud)
    );

};


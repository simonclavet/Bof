
#include "CrappyComp.h"

#include "ZozoComp.h"


void SomeStupidSystemUsingCrappy::DoStuff(ComponentGrid& grid)
{
    //CrappyComp* myCrappy = GET_COMP(grid, CrappyComp, GoodId(123));

    //cout << myCrappy->m_thing << endl;

    //CrappyComp* first = static_cast<CrappyComp*>(grid.GetCompBegin(MyCompTypes::CrappyComp));


    ComponentVector<CrappyComp>& crappyComps = GET_COMPS(grid, CrappyComp);

    const ComponentVector<ZozoComp>& zozoComps = GET_CONST_COMPS(grid, ZozoComp);

    for (int i = 0; i < (int)crappyComps.Size(); i++)
    {
        CrappyComp& crappyComp = crappyComps.GetCompAtIndex(i);
        GoodId entityId = crappyComps.GetEntityAtIndex(i);

        const ZozoComp* zozoComp = zozoComps.GetCompIfExists(entityId);

        if (zozoComp != nullptr)
        {
            crappyComp.m_otherThing += zozoComp->m_someWhackBud.m_crapounet;
        }
    }


    //CrappyComp* second = ++first;

    //cout << second->m_thing << endl;


}
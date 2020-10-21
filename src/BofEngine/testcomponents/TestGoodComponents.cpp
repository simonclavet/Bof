
#include "TestGoodComponents.h"

#include <string>
#include <vector>
#include <map>
//
#include "utils/GoodSave.h"
#include "components/GoodComponents.h"
#include "utils/ringbuffer.h"


#include "CrappyComp.h"
#include "ZozoComp.h"

#include "utils/Timer.h"


class EntityNameComp : public GoodSerializable
{
public:
    string m_name = "noname";

    GOOD_SERIALIZABLE(EntityNameComp, GOOD_VERSION(1), 
        GOOD(m_name));
};


class MetaComponentComp : public GoodSerializable
{
public:
    string m_putSomeInfoAboutComponentsHere = "bof";

    GOOD_SERIALIZABLE(MetaComponentComp, GOOD_VERSION(1), 
        GOOD(m_putSomeInfoAboutComponentsHere));
};


using namespace Timer;


void PrepareMyGridWithComponentVectors(ComponentGrid& grid)
{
    grid.AddCompVector<CrappyComp>();
    grid.AddCompVector<ZozoComp>();

}

void CopyComponentGrid(ComponentGrid& destGrid, const ComponentGrid& srcGrid)
{
    destGrid.m_tagMap = srcGrid.m_tagMap;

    *destGrid.GetCompVector<CrappyComp>() = *srcGrid.GetConstCompVector<CrappyComp>();
    *destGrid.GetCompVector<ZozoComp>() = *srcGrid.GetConstCompVector<ZozoComp>();
}


void TestGoodComponents::Test()
{
    std::string filename = "mydata/whack";

    GoodId tagGenerator = 1000;
    GoodId entityIdGenerator = 10000;

    GoodId happyEntity = entityIdGenerator++;
    GoodId sadEntity = entityIdGenerator++;





    for (GoodFormat format : {GoodFormat::Binary, GoodFormat::Json})
    {
        //MyComponentGridInternal originalGridInternal;
        ComponentGrid myGrid;

        pods::ResizableOutputBuffer out;
        
        PrepareMyGridWithComponentVectors(myGrid);

        {


            CrappyComp* crappyComp = GET_COMPS(myGrid, CrappyComp).AddEntityId(happyEntity);
            crappyComp->m_thing = 123.456f;
            crappyComp->m_otherThing = 3;
            for (int i : { 0, 1 })
            {
                Server& server = crappyComp->m_serverList.m_servers.emplace_back(Server());
                server.m_address = "localhost";
                server.m_port = 8080 + i;
            }
            CrappyComp* crappyComp2 = GET_COMPS(myGrid, CrappyComp).AddEntityId(sadEntity);
            crappyComp2->m_thing = 1101010.01010f;

            for (int i = 0; i < 100; i++)
            {
                CrappyComp* moreCrappyComp = GET_COMPS(myGrid, CrappyComp).AddEntityId(entityIdGenerator++);
                moreCrappyComp->m_thing = 123.456f;
                moreCrappyComp->m_otherThing = 3 + i;
            }


            ZozoComp* zozoComp = GET_COMPS(myGrid, ZozoComp).AddEntityId(happyEntity);
            zozoComp->m_someWhackBud.m_zaza = "bouya";
            zozoComp->m_someIntVector.push_back(543);
            zozoComp->m_someIntVector.push_back(5436);
            zozoComp->m_someIntVector.push_back(5433);
            zozoComp->m_someIntVector.push_back(5433);
            zozoComp->m_someWhackBud.m_crapounet = 1234;

            GoodId superDuperTag = tagGenerator++; // needs to be higher than CompTypes::Count... and under 

            GET_TAGS(myGrid, superDuperTag).AddEntityId(happyEntity);
            GET_TAGS(myGrid, superDuperTag).AddEntityId(sadEntity);


            ComponentGrid savedCompGrid;
            PrepareMyGridWithComponentVectors(savedCompGrid);
            CopyComponentGrid(savedCompGrid, myGrid);
            
            cout << savedCompGrid.Equals(myGrid) << endl;

            GET_COMPS(savedCompGrid, CrappyComp).AddEntityId(123344);

            cout << savedCompGrid.Equals(myGrid) << endl;


            SomeStupidSystemUsingCrappy::DoStuff(myGrid);

            //cout << savedCompGrid.Equals(myGrid) << endl;

            {
                TIMED << "Writing to " << magic_enum::enum_name(format) << " file... ";
                GoodHelpers::WriteToFile(myGrid, filename, format);

                //switch (format)
                //{
                //case GoodFormat::Binary:
                //{
                //    pods::BinarySerializer<decltype(out)> serializer(out);
                //    serializer.save(originalGridInternal);
                //    break;
                //}
                //case GoodFormat::Json:
                //{
                //    pods::PrettyJsonSerializer<decltype(out)> serializer(out);
                //    serializer.save(originalGridInternal);
                //    break;
                //}
                //}

            }
        }

        {
            ComponentGrid loadedGrid;
            PrepareMyGridWithComponentVectors(loadedGrid);

            {
                TIMED << "Reading from " << magic_enum::enum_name(format) << " file... ";
                GoodHelpers::ReadFromFile(loadedGrid, filename, format);
                //loadedGridInternal.PostDeserialize();


                //pods::InputBuffer in(out.data(), out.size());

                //switch (format)
                //{
                //case GoodFormat::Binary:
                //{
                //    pods::BinaryDeserializer<decltype(in)> deserializer(in);
                //    deserializer.load(loadedGridInternal);
                //    break;
                //}
                //case GoodFormat::Json:
                //{
                //    pods::JsonDeserializer<decltype(in)> deserializer(in);
                //    deserializer.load(loadedGridInternal);
                //    break;
                //}
                //}
            }

            //loadedGridInternal.PostDeserialize();

            assert(myGrid.Equals(loadedGrid));



            const CrappyComp* crappyComp = GET_COMPS(loadedGrid, CrappyComp).GetCompIfExists(happyEntity);
            cout << crappyComp->m_thing << endl;


            const ZozoComp* zozoComp = GET_COMPS(loadedGrid, ZozoComp).GetCompIfExists(happyEntity);
            cout << zozoComp->m_someWhackBud.m_zaza << endl;
            cout << zozoComp->m_someIntVector[0] << endl;
            cout << zozoComp->m_someWhackBud.m_crapounet << endl;
        }
    }












    //vector<CrappyComp> ccomps = CrappyComp::CreateVector();

    //for (GoodFormat format : {GoodFormat::Binary})//, GoodFormat::Json})
    //{

    //    ServerList serverList;
    //    {
    //        TIMED << "Creating object... ";
    //        for (int i = 0; i < 1000000; i++)
    //        {
    //            serverList.m_servers.push_back({ "bonjour", 123 + i });
    //        }
    //    }
    //    {
    //        TIMED << "Writing to " << magic_enum::enum_name(format) << " file... ";
    //        GoodHelpers::WriteToFile(serverList, filename, format);
    //    }


    //    ServerList loadedServerList;
    //    {
    //        TIMED << "Reading from " << magic_enum::enum_name(format) << " file... ";
    //        GoodHelpers::ReadFromFile(loadedServerList, filename, format);
    //    }
    //    cout << loadedServerList.m_servers.back().m_port << endl;
    //}
    

    //// Heres how to make a compgrid. First make our data, give it to the interface.
    //// That's a way to make sure component and system code doesn't have to include the whole codebase
    //MyComponentGridInternal* myComponentGridInternal = new MyComponentGridInternal();
    //m_myGrid = MyCompGridOld(myComponentGridInternal);
    //
    //// But don't have to: just keep in on the stack!
    //MyComponentGridInternal otherGridInternal;
    //MyCompGrid otherGrid(&otherGridInternal);
    //



    //
    //// make grids. add components. run a system. save and load in binary and json
    //for (GoodFormat format : {GoodFormat::Binary, GoodFormat::Json})
    //{
    //    MyComponentGridInternal originalGridInternal;

    //    pods::ResizableOutputBuffer out;

    //    {
    //        MyCompGrid myGrid(&originalGridInternal);

    //        // prepare the component metadata, which is actually a component on the entity with id identical to the component id





    //        CrappyComp* crappyComp = GET_COMPS(myGrid, CrappyComp).AddEntityId(happyEntity);
    //        crappyComp->m_thing = 123.456f;
    //        crappyComp->m_otherThing = 3;
    //        for (int i : { 0, 1 })
    //        {
    //            Server& server = crappyComp->m_serverList.m_servers.emplace_back(Server());
    //            server.m_address = "localhost";
    //            server.m_port = 8080;
    //        }
    //        CrappyComp* crappyComp2 = GET_COMPS(myGrid, CrappyComp).AddEntityId(sadEntity);
    //        crappyComp2->m_thing = 1101010.01010f;

    //        for (int i = 0; i < 90000; i++)
    //        {
    //            CrappyComp* moreCrappyComp = GET_COMPS(myGrid, CrappyComp).AddEntityId(entityIdGenerator++);
    //            moreCrappyComp->m_thing = 123.456f;
    //            moreCrappyComp->m_otherThing = 3 + i;
    //            //moreCrappyComp->m_serverList.m_servers =
    //            //{
    //            //    Server { "localhost", 1234 },
    //            //    Server { "my.com", 2018 }
    //            //};

    //        }


    //        ZozoComp* zozoComp = GET_COMPS(myGrid, ZozoComp).AddEntityId(happyEntity);
    //        zozoComp->m_someWhackBud.m_zaza = "bouya";
    //        zozoComp->m_someIntVector.push_back(543);
    //        zozoComp->m_someIntVector.push_back(543);
    //        zozoComp->m_someIntVector.push_back(543);
    //        zozoComp->m_someIntVector.push_back(543);
    //        zozoComp->m_someWhackBud.m_crapounet = 1234;

    //        GoodId superDuperTag = tagGenerator++; // needs to be higher than CompTypes::Count... and under 

    //        GET_TAGS(myGrid, superDuperTag).AddEntityId(happyEntity);
    //        GET_TAGS(myGrid, superDuperTag).AddEntityId(sadEntity);


    //        MyComponentGridInternal savedCompGrid = originalGridInternal;
    //        cout << savedCompGrid.Equals(originalGridInternal) << endl;

    //        GET_COMPS(savedCompGrid, CrappyComp).AddEntityId(123344);


    //        SomeStupidSystemUsingCrappy::DoStuff(myGrid);

    //        cout << savedCompGrid.Equals(originalGridInternal) << endl;

    //        {
    //            TIMED << "Writing to " << magic_enum::enum_name(format) << " file... ";
    //            GoodHelpers::WriteToFile(originalGridInternal, filename, format);

    //            //switch (format)
    //            //{
    //            //case GoodFormat::Binary:
    //            //{
    //            //    pods::BinarySerializer<decltype(out)> serializer(out);
    //            //    serializer.save(originalGridInternal);
    //            //    break;
    //            //}
    //            //case GoodFormat::Json:
    //            //{
    //            //    pods::PrettyJsonSerializer<decltype(out)> serializer(out);
    //            //    serializer.save(originalGridInternal);
    //            //    break;
    //            //}
    //            //}

    //        }
    //    }

    //    {
    //        MyComponentGridInternal loadedGridInternal;

    //        {
    //            TIMED << "Reading from " << magic_enum::enum_name(format) << " file... ";
    //            GoodHelpers::ReadFromFile(loadedGridInternal, filename, format);
    //            loadedGridInternal.PostDeserialize();


    //            //pods::InputBuffer in(out.data(), out.size());

    //            //switch (format)
    //            //{
    //            //case GoodFormat::Binary:
    //            //{
    //            //    pods::BinaryDeserializer<decltype(in)> deserializer(in);
    //            //    deserializer.load(loadedGridInternal);
    //            //    break;
    //            //}
    //            //case GoodFormat::Json:
    //            //{
    //            //    pods::JsonDeserializer<decltype(in)> deserializer(in);
    //            //    deserializer.load(loadedGridInternal);
    //            //    break;
    //            //}
    //            //}
    //        }

    //        loadedGridInternal.PostDeserialize();

    //        assert(originalGridInternal.Equals(loadedGridInternal));


    //        MyCompGrid loadedGrid(&loadedGridInternal);


    //        const CrappyComp* crappyComp = GET_COMPS(loadedGrid, CrappyComp).GetCompIfExists(happyEntity);
    //        cout << crappyComp->m_thing << endl;


    //        const ZozoComp* zozoComp = GET_COMPS(loadedGrid, ZozoComp).GetCompIfExists(happyEntity);
    //        cout << zozoComp->m_someWhackBud.m_zaza << endl;
    //        cout << zozoComp->m_someIntVector[0] << endl;
    //        cout << zozoComp->m_someWhackBud.m_crapounet << endl;
    //    }
    //}

}

//
//template <typename T>
//typename remove_reference<T>::type&& move(T&& arg)
//{
//    return static_cast<typename remove_reference<T>::type&&>(arg);
//}




int notmain(int /*argc*/, char** /*argv*/)
{
    //RingBuffer<string, 3> rb;
    //rb.Push() = "hallo";

    //cout << rb.GetCurrent() << endl;

   
    //rb.Push() = "bof";
    //rb.Push() = "bofe";
    //rb.Push() = "bofee";
    //rb.Push() = "bofgas";

    //cout << rb.GetPastItem(3) << endl;

    TestGoodComponents t;
    t.Test();

    return EXIT_SUCCESS;
}




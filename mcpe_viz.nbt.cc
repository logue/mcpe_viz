/*
  Minecraft Pocket Edition (MCPE) World File Visualization & Reporting Tool
  (c) Plethora777, 2015.9.26

  GPL'ed code - see LICENSE

  NBT support
*/

#include <stdio.h>
#include <fstream>
#include <algorithm>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <cmath>
#endif

#include "mcpe_viz.util.h"
#include "mcpe_viz.h"
#include "mcpe_viz.nbt.h"

namespace mcpe_viz {

  std::string makeGeojsonHeader(double ix, double iy, bool adjustCoordFlag) {
    char tmpstring[256];

    std::string s =
      "{"
      "\"type\":\"Feature\","
      "\"geometry\":{\"type\":\"Point\",\"coordinates\":["
      ;
    if ( std::isnan(ix) || std::isnan(iy) ) {
      // we don't put out anything because "NaN" is not valid JSON
    } else {
      // adjust position so that items are in the center of pixels
      // todobig - play with this to see if we can make it better
      if ( adjustCoordFlag ) {
        ix += 0.5; //todobig - plus or minus here? hmm
        iy += 0.5;
      }
      sprintf(tmpstring,"%.1lf,%.1lf",ix,iy);
      s += tmpstring;
    }
    s +=
      "]},"
      "\"properties\":{"
      ;
    return s;
  }
  
  std::string makeGeojsonHeader_MultiPoint(int n, double *ix, double *iy) {
    char tmpstring[256];

    std::string s =
      "{"
      "\"type\":\"Feature\","
      "\"geometry\":{\"type\":\"MultiPoint\",\"coordinates\":["
      ;

    for (int i=0; i < n; i++) {

      // adjust position so that items are in the center of pixels
      // todobig - play with this to see if we can make it better
      if ( true ) {
        ix[i] += 0.5;
        iy[i] += 0.5;
      }
      
      sprintf(tmpstring,"[%.1lf,%.1lf]",ix[i],iy[i]);
      s += tmpstring;
      if ( i < (n-1) ) {
        s += ",";
      }
    }

    s +=
      "]},"
      "\"properties\":{"
      ;
    return s;
  }
  
  // nbt parsing helpers
  int32_t globalNbtListNumber=0;
  int32_t globalNbtCompoundNumber=0;

  int32_t parseNbtTag( const char* hdr, int& indent, const MyNbtTag& t ) {

    logger.msg(kLogInfo1,"%s[%s] ", makeIndent(indent,hdr).c_str(), t.first.c_str());

    nbt::tag_type tagType = t.second->get_type();
      
    switch ( tagType ) {
    case nbt::tag_type::End:
      logger.msg(kLogInfo1,"TAG_END\n");
      break;
    case nbt::tag_type::Byte:
      {
        nbt::tag_byte v = t.second->as<nbt::tag_byte>();
        logger.msg(kLogInfo1,"%d 0x%x (byte)\n", v.get(), v.get());
      }
      break;
    case nbt::tag_type::Short:
      {
        nbt::tag_short v = t.second->as<nbt::tag_short>();
        logger.msg(kLogInfo1,"%d 0x%x (short)\n", v.get(), v.get());
      }
      break;
    case nbt::tag_type::Int:
      {
        nbt::tag_int v = t.second->as<nbt::tag_int>();
        logger.msg(kLogInfo1,"%d 0x%x (int)\n", v.get(), v.get());
      }
      break;
    case nbt::tag_type::Long:
      {
        nbt::tag_long v = t.second->as<nbt::tag_long>();
        // note: silly work around for linux vs win32 weirdness
        logger.msg(kLogInfo1,"%lld 0x%llx (long)\n", (long long int)v.get(), (long long int)v.get());
      }
      break;
    case nbt::tag_type::Float:
      {
        nbt::tag_float v = t.second->as<nbt::tag_float>();
        logger.msg(kLogInfo1,"%f (float)\n", v.get());
      }
      break;
    case nbt::tag_type::Double:
      {
        nbt::tag_double v = t.second->as<nbt::tag_double>();
        logger.msg(kLogInfo1,"%lf (double)\n", v.get());
      }
      break;
    case nbt::tag_type::Byte_Array:
      {
        nbt::tag_byte_array v = t.second->as<nbt::tag_byte_array>();
        logger.msg(kLogInfo1,"[");
        int32_t i=0;
        for (const auto& itt: v ) {
          if ( i++ > 0 ) { logger.msg(kLogInfo1," "); }
          logger.msg(kLogInfo1,"%02x", (int)itt);
        }
        logger.msg(kLogInfo1,"] (hex byte array)\n");
      }
      break;
    case nbt::tag_type::String:
      {
        nbt::tag_string v = t.second->as<nbt::tag_string>();
        logger.msg(kLogInfo1,"'%s' (string)\n", v.get().c_str());
      }
      break;
    case nbt::tag_type::List:
      {
        nbt::tag_list v = t.second->as<nbt::tag_list>();
        int32_t lnum = ++globalNbtListNumber;
        logger.msg(kLogInfo1,"LIST-%d {\n",lnum);
        indent++;
        for ( const auto& it: v ) {
          parseNbtTag( hdr, indent, std::make_pair(std::string(""), it.get().clone() ) );
        }
        if ( --indent < 0 ) { indent=0; }
        logger.msg(kLogInfo1,"%s} LIST-%d\n", makeIndent(indent,hdr).c_str(), lnum);
      }
      break;
    case nbt::tag_type::Compound:
      {
        nbt::tag_compound v = t.second->as<nbt::tag_compound>();
        int32_t cnum = ++globalNbtCompoundNumber;
        logger.msg(kLogInfo1,"COMPOUND-%d {\n",cnum);
        indent++;
        for ( const auto& it: v ) {
          parseNbtTag( hdr, indent, std::make_pair( it.first, it.second.get().clone() ) );
        }
        if ( --indent < 0 ) { indent=0; }
        logger.msg(kLogInfo1,"%s} COMPOUND-%d\n", makeIndent(indent,hdr).c_str(),cnum);
      }
      break;
    case nbt::tag_type::Int_Array:
      {
        nbt::tag_int_array v = t.second->as<nbt::tag_int_array>();
        logger.msg(kLogInfo1,"[");
        int32_t i=0;
        for ( const auto& itt: v ) {
          if ( i++ > 0 ) { logger.msg(kLogInfo1," "); }
          logger.msg(kLogInfo1,"%x", itt);
        }
        logger.msg(kLogInfo1,"] (hex int array)\n");
      }
      break;
    default:
      logger.msg(kLogInfo1,"[ERROR: Unknown tag type = %d]\n", (int)tagType);
      break;
    }

    return 0;
  }

    
  int32_t parseNbt( const char* hdr, const char* buf, int32_t bufLen, MyNbtTagList& tagList ) {
    int32_t indent=0;
    logger.msg(kLogInfo1,"%sNBT Decode Start\n",makeIndent(indent,hdr).c_str());

    // these help us look at dumped nbt data and match up LIST's and COMPOUND's
    globalNbtListNumber=0;
    globalNbtCompoundNumber=0;
      
    std::istringstream is(std::string(buf,bufLen));
    nbt::io::stream_reader reader(is, endian::little);

    // remove all elements from taglist
    tagList.clear();
      
    // read all tags
    MyNbtTag t;
    bool done = false;
    std::istream& pis = reader.get_istr();
    while ( !done && (pis) && (!pis.eof()) ) {
      try {
        // todo emplace_back?
        tagList.push_back(reader.read_tag());
      }
      catch (std::exception& e) {
        // check for eof which means all is well
        if ( ! pis.eof() ) {
          fprintf(stderr, "NBT exception: (%s) (eof=%s) (is=%s)\n"
                  , e.what()
                  , pis.eof() ? "true" : "false"
                  , (pis) ? "true" : "false"
                  );
        }
        done = true;
      }
    }
      
    // iterate over the tags
    for ( const auto& itt: tagList ) {
      parseNbtTag( hdr, indent, itt );
    }
      
    logger.msg(kLogInfo1,"%sNBT Decode End (%d tags)\n",makeIndent(indent,hdr).c_str(), (int)tagList.size());

    return 0;
  }


  
  // todo - use this throughout parsing or is it madness? :)
  template<class T>
  class MyValue {
  public:
    T value;
    bool valid;
    MyValue() {
      clear();
    }
    void clear() {
      value=(T)0;
      valid=false;
    }
    void set(T nvalue) {
      value = nvalue;
      valid = true;
    }
    std::string toString() {
      if ( valid ) {
        return std::string(""+value);
      } else {
        return std::string("*Invalid Value*");
      }
    }
  };



  template<class T>
  class Point2d {
  public:
    T x, y;
    bool valid;
    Point2d() {
      clear();
    }
    void set(T nx, T ny) {
      x=nx;
      y=ny;
      valid=true;
    }
    void clear(){
      x=(T)0;
      y=(T)0;
      valid=false;
    }
    bool isValid() {
      return ! ( std::isnan(x) || std::isnan(y) );
    }
    std::string toGeoJSON() {
      // todo - how to report invalid in geojson?
      // if ( valid ) {
      std::ostringstream str;
      if ( std::isnan(x) || std::isnan(y) ) {
        // we don't put out anything because "NaN" is not valid JSON
        str << "";
      } else {
        str << x << "," << y;
      }
      return str.str();
    }
    std::string toString() {
      if ( valid ) {
        std::ostringstream str;
        str << x << "," << y;
        return str.str();
      } else {
        return std::string("*Invalid-Point2d*");
      }
    }
  };



  template<class T>
  class Point3d {
  public:
    T x, y, z;
    bool valid;
    Point3d() {
      clear();
    }
    void set(T nx, T ny, T nz) {
      x=nx;
      y=ny;
      z=nz;
      valid=true;
    }
    void clear(){
      x=(T)0;
      y=(T)0;
      z=(T)0;
      valid=false;
    }
    bool isValid() {
      return ! ( std::isnan(x) || std::isnan(y) || std::isnan(z) );
    }
    std::string toGeoJSON() {
      // todo - how to report invalid in geojson?
      // if ( valid ) {
      std::ostringstream str;
      if ( std::isnan(x) || std::isnan(y) || std::isnan(z) ) {
        // we don't put out anything because "NaN" is not valid JSON
        str << "";
      } else {
        str << x << "," << y << "," << z;
      }
      return str.str();
    }
    std::string toString() {
      if ( valid ) {
        std::ostringstream str;
        str << x << ", " << y << ", " << z;
        return str.str();
      } else {
        return std::string("*Invalid-Point2d*");
      }
    }
    std::string toStringImageCoords(int32_t dimId) {
      if ( valid ) {
        double tix, tiy;
        worldPointToImagePoint(dimId, x,z, tix,tiy, false);
        int32_t ix = tix;
        int32_t iy = tiy;
        std::ostringstream str;
        str << ix << ", " << iy;
        return str.str();
      } else {
        return std::string("*Invalid-Point2d*");
      }
    }
    std::string toStringWithImageCoords(int32_t dimId) {
      return std::string("(" + toString() + " @ image " + toStringImageCoords(dimId) + ")");
    }
  };



  // todo - each value in these structs should be an object that does "valid" checking
  class ParsedEnchantment {
  public:
    MyValue<int32_t> id;
    MyValue<int32_t> level;
    ParsedEnchantment() {
      clear();
    }
    void clear() {
      id.clear();
      level.clear();
    }
    int32_t parse(nbt::tag_compound& iench) {
      if ( iench.has_key("id", nbt::tag_type::Short) ) {
        id.set( iench["id"].as<nbt::tag_short>().get() );
      }
      if ( iench.has_key("lvl", nbt::tag_type::Short) ) {
        level.set( iench["lvl"].as<nbt::tag_short>().get() );
      }
      return 0;
    }
    std::string toGeoJSON() {
      char tmpstring[1025];
      std::string s = "";
      if ( id.valid ) {
        s += "\"Name\":\"";
        if ( has_key(enchantmentInfoList, id.value) ) {
          s += enchantmentInfoList[id.value]->name;
        } else {
          sprintf(tmpstring,"(UNKNOWN: id=%d 0x%x)",id.value,id.value);
          s += tmpstring;
        }
        //s += "\"Level\":\"";
        sprintf(tmpstring," (%d)\"", level.value);
        s += tmpstring;
      } else {
        s += "\"valid\":\"false\"";
      }
      return s;
    }
    std::string toString() {
      char tmpstring[1025];
      std::string s = "";
      if ( id.valid ) {
        if ( has_key(enchantmentInfoList, id.value) ) {
          s += enchantmentInfoList[id.value]->name;
        } else {
          sprintf(tmpstring,"(UNKNOWN: id=%d 0x%x)",id.value,id.value);
          s += tmpstring;
        }
        sprintf(tmpstring," (%d)", level.value);
        s += tmpstring;
      } else {
        s += "*Invalid id*";
      }
      return s;
    }
  };
    


  class ParsedItem {
  public:
    bool valid;
    bool armorFlag;
    int32_t id;
    int32_t slot;
    int32_t damage;
    int32_t count;
    int32_t repairCost;
    std::vector< std::unique_ptr<ParsedEnchantment> > enchantmentList;
    ParsedItem() {
      clear();
    }
    void clear() {
      valid = false;
      armorFlag = false;
      id = -1;
      slot = -1;
      damage = -1;
      count = -1;
      repairCost = -1;
      enchantmentList.clear();
    }
    int32_t parse(nbt::tag_compound& iitem) {
      if ( iitem.has_key("Count", nbt::tag_type::Byte) ) {
        count = iitem["Count"].as<nbt::tag_byte>().get();
      }
      if ( iitem.has_key("Damage", nbt::tag_type::Short) ) {
        damage = iitem["Damage"].as<nbt::tag_short>().get();
      }
      if ( iitem.has_key("Slot", nbt::tag_type::Byte) ) {
        slot = iitem["Slot"].as<nbt::tag_byte>().get();
      }
      if ( iitem.has_key("id", nbt::tag_type::Short) ) {
        id = iitem["id"].as<nbt::tag_short>().get();
      }

      // todo - other fields? 

      // look for item enchantment
      if ( iitem.has_key("tag", nbt::tag_type::Compound) ) {
        nbt::tag_compound etag = iitem["tag"].as<nbt::tag_compound>();

        if ( etag.has_key("RepairCost", nbt::tag_type::Int) ) {
          repairCost = etag["RepairCost"].as<nbt::tag_int>().get();
        }

        if ( etag.has_key("ench", nbt::tag_type::List) ) {
          nbt::tag_list elist = etag["ench"].as<nbt::tag_list>();

          for ( const auto& it: elist ) {
            nbt::tag_compound ench = it.as<nbt::tag_compound>();
            std::unique_ptr<ParsedEnchantment> e(new ParsedEnchantment());
            int32_t ret = e->parse(ench);
            if ( ret == 0 ) {
              enchantmentList.push_back( std::move(e) );
            } else {
              // todo err?
            }
          }
        }
      }
      valid = true;
      return 0;
    }
    
    int32_t parseArmor(nbt::tag_compound& iarmor) {
      armorFlag = true;
      return parse(iarmor);
    }

    std::string toGeoJSON(bool swallowFlag=false, int32_t swallowValue=0, bool showCountFlag=false) {
      std::vector<std::string> list;
      std::string s;
      char tmpstring[1025];
        
      if ( ! valid ) { return std::string(""); }
        
      if ( swallowFlag ) {
        if ( swallowValue == id ) {
          return std::string("");
        }
      }
        
      s = "\"Name\":";
      if ( id >= 0 && id <= 255 ) {
        s += "\"" + getBlockName(id,damage) + "\"";
      } else {
        std::string iname = getItemName(id, damage);
        s += "\"" + iname + "\"";
      }
      list.push_back(s);

      // todo - not useful?
      if ( false ) {
        if ( damage >= 0 ) {
          sprintf(tmpstring,"\"Damage\":\"%d\"", damage);
          list.push_back(std::string(tmpstring));
        }
        if ( slot >= 0 ) {
          sprintf(tmpstring,"\"Slot\":\"%d\"", slot);
          list.push_back(std::string(tmpstring));
        }
      }

      if ( showCountFlag && count >= 0 ) {
        sprintf(tmpstring,"\"Count\":\"%d\"", count);
        list.push_back(std::string(tmpstring));
      }
        
      if ( enchantmentList.size() > 0 ) {
        s = "\"Enchantments\":[";
        int32_t i = enchantmentList.size();
        for ( const auto& it: enchantmentList ) {
          s+="{" + it->toGeoJSON() + "}";
          if ( --i > 0 ) {
            s += ",";
          }
        }
        s += "]";
        list.push_back(s);
      }

      // check for icon image
      char urlImage[1025];
      if ( id < 256 ) {
        sprintf(urlImage,"images/mcpe_viz.block.%d.%d.png", id, damage);
      } else {
        sprintf(urlImage,"images/mcpe_viz.item.%d.%d.png", id, damage);
      }

      if ( ! file_exists(urlImage) ) {
        // check for non-variant
        if ( id < 256 ) {
          sprintf(urlImage,"images/mcpe_viz.block.%d.%d.png", id, 0);
        } else {
          sprintf(urlImage,"images/mcpe_viz.item.%d.%d.png", id, 0);
        }
      }
      
      if ( file_exists(dirExec + "/" + urlImage) ) {
        std::string fImage(urlImage);
        int32_t imgId = -1;
        if ( has_key(imageFileMap, fImage) ) {
          imgId = imageFileMap[fImage];
        } else {
          imgId = globalIconImageId++;
          imageFileMap.insert( std::make_pair(fImage,imgId) );
        }
        sprintf(tmpstring,"\"imgid\":%d",imgId);
        list.push_back(tmpstring);
      }
      

      // combine the list and put the commas in the right spots (stupid json)
      s = "";
      int32_t i=list.size();
      for ( const auto& iter: list ) {
        s += iter;
        if ( --i > 0 ) {
          s += ",";
        }
      }
        
      return s;
    }
      
    std::string toString(bool swallowFlag=false, int32_t swallowValue=0) {
      char tmpstring[1025];

      if ( ! valid ) { return std::string("*Invalid Item*"); }

      if ( swallowFlag ) {
        if ( swallowValue == id ) {
          return std::string("");
        }
      }
        
      std::string s = "[";

      if ( id >= 0 && id <= 255 ) {
        s += "Block:" + getBlockName(id,damage);
      } else {
        s += "Item:" + getItemName(id, damage);
      }

      if ( damage >= 0 ) {
        sprintf(tmpstring," Damage=%d", damage);
        s += tmpstring;
      }
      if ( count >= 0 ) {
        sprintf(tmpstring," Count=%d", count);
        s += tmpstring;
      }
      if ( slot >= 0 ) {
        sprintf(tmpstring," Slot=%d", slot);
        s += tmpstring;
      }

      if ( enchantmentList.size() > 0 ) {
        s += " Enchantments=[";
        int32_t i=enchantmentList.size();
        for ( const auto& it: enchantmentList ) {
          s += it->toString();
          if ( --i > 0 ) {
            s += "; "; 
          }
        }
        s += "]";
      }
        
      s += "]";
      return s;
    }
  };
      


  class ParsedEntity {
  public:
    Point3d<int> bedPosition;
    Point3d<int> spawn;
    Point3d<double> pos;
    Point2d<double> rotation;
    int32_t idShort;
    int32_t idFull;
    int32_t tileId;
    int32_t dimensionId;
    bool playerLocalFlag;
    bool playerRemoteFlag;
    std::string playerType;
    std::string playerId;
    std::vector< std::unique_ptr<ParsedItem> > inventory;
    std::vector< std::unique_ptr<ParsedItem> > enderchest;
    std::vector< std::unique_ptr<ParsedItem> > armorList;
    ParsedItem itemInHand;
    ParsedItem item;
    // todobig - this is very handy + powerful - use it for other Parsed classes?
    std::vector< std::pair<std::string,std::string> > otherProps;
    bool otherPropsSortedFlag;
      
    ParsedEntity() {
      clear();
    }
    void clear() {
      bedPosition.clear();
      spawn.clear();
      pos.clear();
      rotation.clear();
      idShort = 0;
      idFull = 0;
      tileId = -1;
      dimensionId = -1;
      playerLocalFlag = false;
      playerRemoteFlag = false;
      playerType = "";
      playerId = "";
      inventory.clear();
      enderchest.clear();
      armorList.clear();
      itemInHand.clear();
      item.clear();
      otherProps.clear();
      otherPropsSortedFlag = false;
    }
    int32_t addInventoryItem ( nbt::tag_compound &iitem ) {
      std::unique_ptr<ParsedItem> it(new ParsedItem());
      int32_t ret = it->parse(iitem);
      inventory.push_back( std::move(it) );
      return ret;
    }
    int32_t addEnderChestItem ( nbt::tag_compound &iitem ) {
      std::unique_ptr<ParsedItem> it(new ParsedItem());
      int32_t ret = it->parse(iitem);
      enderchest.push_back( std::move(it) );
      return ret;
    }
    int32_t doItemInHand ( nbt::tag_compound &iitem ) {
      itemInHand.clear();
      return itemInHand.parse(iitem);
    }
    int32_t doItem ( nbt::tag_compound &iitem ) {
      item.clear();
      return item.parse(iitem);
    }
    int32_t doTile ( int32_t ntileId ) {
      tileId = ntileId;
      return 0;
    }
    
    int32_t addArmor ( nbt::tag_compound &iarmor ) {
      std::unique_ptr<ParsedItem> armor(new ParsedItem());
      int32_t ret = armor->parseArmor(iarmor);
      if ( ret == 0 ) {
        armorList.push_back( std::move(armor) );
      }
      return 0;
    }

    int32_t addOtherProp(const std::string& key, const std::string& value) {
      otherProps.push_back( std::make_pair(key,value) );
      return 0;
    }
      
    int32_t checkOtherProp(nbt::tag_compound& tc, const std::string& key) {
      char tmpstring[1025];
      if ( tc.has_key(key)) {
        // seems silly that we have to do this:
        const nbt::tag_type tagType = tc[key].get_type();
        switch ( tagType ) {
        case nbt::tag_type::Byte:
          {
            int8_t v = tc[key].as<nbt::tag_byte>().get();
            sprintf(tmpstring,"%d (0x%x)", v, v);
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::Short:
          {
            int16_t v = tc[key].as<nbt::tag_short>().get();
            sprintf(tmpstring,"%d (0x%x)", v, v);
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::Int:
          {
            int32_t v = tc[key].as<nbt::tag_int>().get();
            sprintf(tmpstring,"%d (0x%x)", v, v);
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::Long:
          {
            int64_t v = tc[key].as<nbt::tag_long>().get();
            sprintf(tmpstring,"%lld (0x%llx)", (long long int)v, (long long int)v);
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::Float:
          {
            sprintf(tmpstring,"%f",tc[key].as<nbt::tag_float>().get());
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::Double:
          {
            sprintf(tmpstring,"%lf",tc[key].as<nbt::tag_double>().get());
            addOtherProp(key, std::string(tmpstring));
          }
          break;
        case nbt::tag_type::String:
          {
            addOtherProp(key, std::string(tc[key].as<nbt::tag_string>().get()));
          }
          break;
        default:
          // todo - err?
          // items we don't parse (yet)
          //Byte_Array = 7,
          //List = 9,
          //Compound = 10,
          //Int_Array = 11,
          fprintf(stderr,"WARNING: Unable to capture entity other prop key=%s\n", key.c_str());
          break;
        }
      }
      return 0;
    }

    std::string toGeoJSON(int32_t forceDimensionId) {
      std::vector<std::string> list;
      std::string s = "";
      char tmpstring[1025];

      // we make sure that the entity is valid -- have seen rare occurances of mobs with "nan" in pos et al
      if ( ! pos.isValid() || ! rotation.isValid() ) {
        logger.msg(kLogInfo1,"WARNING: Not outputting geojson for mob with invalid position/rotation\n");
        return "";
      }
      
      double ix, iy;
      worldPointToGeoJSONPoint(forceDimensionId, pos.x,pos.z, ix,iy);
      s += makeGeojsonHeader(ix,iy);

      if ( has_key(entityInfoList, idShort) ) {
        sprintf(tmpstring,"\"Name\":\"%s\"", entityInfoList[idShort]->name.c_str());
        list.push_back(std::string(tmpstring));
        sprintf(tmpstring,"\"etype\":\"%s\"", entityInfoList[idShort]->etype.c_str());
        list.push_back(std::string(tmpstring));
      } else {
        sprintf(tmpstring,"\"Name\":\"*UNKNOWN: id=%d 0x%x\"", idShort,idShort);
        list.push_back(std::string(tmpstring));
      }

      sprintf(tmpstring,"\"id\":\"%d\"", idShort);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring,"\"idFull\":\"%d\"", idFull);
      list.push_back(std::string(tmpstring));

      // todo - needed?
      if ( playerLocalFlag || playerRemoteFlag ) {
        list.push_back(std::string("\"player\":\"true\""));

        sprintf(tmpstring,"\"playerType\":\"%s\"", playerType.c_str());
        list.push_back(std::string(tmpstring));

        sprintf(tmpstring,"\"playerId\":\"%s\"", playerId.c_str());
        list.push_back(std::string(tmpstring));

        if ( has_key(playerIdToName, playerId) ) {
          sprintf(tmpstring,"\"playerName\":\"%s\"", playerIdToName[playerId].c_str());
          list.push_back(std::string(tmpstring));
        } else {
          sprintf(tmpstring,"\"playerName\":\"*UNKNOWN*\"");
          list.push_back(std::string(tmpstring));
        }
      } else {
        // list.push_back(std::string("\"player\":\"false\""));
      }

      if ( forceDimensionId >= 0 ) {
        // getting dimension name from myWorld is more trouble than it's worth here :)
        sprintf(tmpstring,"\"Dimension\":\"%d\"", forceDimensionId);
        list.push_back(std::string(tmpstring));
      }

      sprintf(tmpstring, "\"Pos\":[%s]", pos.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"Rotation\":[%s]", rotation.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));
        
      if ( playerLocalFlag || playerRemoteFlag ) {
        sprintf(tmpstring,"\"BedPos\":[%s]", bedPosition.toGeoJSON().c_str());
        list.push_back(std::string(tmpstring));
        sprintf(tmpstring,"\"Spawn\":[%s]", spawn.toGeoJSON().c_str());
        list.push_back(std::string(tmpstring));
      }

      if ( armorList.size() > 0 ) {
        std::vector<std::string> tlist;
        for ( const auto& it: armorList ) {
          std::string sarmor = it->toGeoJSON(true,0,false);
          if ( sarmor.size() > 0 ) {
            tlist.push_back(std::string("{" + sarmor + "}"));
          }
        }
        if ( tlist.size() > 0 ) {
          std::string ts = "\"Armor\":[";
          int32_t i = tlist.size();
          for (const auto& iter : tlist ) {
            ts += iter;
            if ( --i > 0 ) {
              ts += ",";
            }
          }
          ts += "]";
          list.push_back(ts);
        }
      }

      if ( inventory.size() > 0 ) {
        std::vector<std::string> tlist;
        for ( const auto& it: inventory ) {
          std::string sitem = it->toGeoJSON(true,0,true);
          if ( sitem.size() > 0 ) {
            tlist.push_back(std::string("{" + sitem + "}"));
          }
        }
        if ( tlist.size() > 0 ) {
          std::string ts = "\"Inventory\":[";
          int32_t i = tlist.size();
          for (const auto& iter : tlist ) {
            ts += iter;
            if ( --i > 0 ) {
              ts += ",";
            }
          }
          ts += "]";
          list.push_back(ts);
        }
      }

      if ( enderchest.size() > 0 ) {
        std::vector<std::string> tlist;
        for ( const auto& it: enderchest ) {
          std::string sitem = it->toGeoJSON(true,0,true);
          if ( sitem.size() > 0 ) {
            tlist.push_back(std::string("{" + sitem + "}"));
          }
        }
        if ( tlist.size() > 0 ) {
          std::string ts = "\"EnderChest\":[";
          int32_t i = tlist.size();
          for (const auto& iter : tlist ) {
            ts += iter;
            if ( --i > 0 ) {
              ts += ",";
            }
          }
          ts += "]";
          list.push_back(ts);
        }
      }
      
      if ( itemInHand.valid ) {
        list.push_back(std::string("\"ItemInHand\":{" + itemInHand.toGeoJSON() + "}"));
      }
        
      if ( item.valid ) {
        list.push_back(std::string("\"Item\":{" + item.toGeoJSON() + "}"));
      }

      if ( ! otherPropsSortedFlag ) {
        std::sort( otherProps.begin(), otherProps.end() );
        otherPropsSortedFlag = true;
      }
      for ( const auto& it : otherProps ) {
        list.push_back(std::string("\"" + it.first + "\":\"" + it.second + "\""));
      }
        
      // todo?
      /*
        if ( tileId >= 0 ) {
        sprintf(tmpstring," Tile=[%s (%d 0x%x)]", blockInfoList[tileId].name.c_str(), tileId, tileId);
        s += tmpstring;
        }
      */

      if ( list.size() > 0 ) {
        list.push_back(std::string("\"Entity\":\"true\""));
        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
      }
      s += "}}";
        
      return s;
    }
      
    // todo - this should probably be multi-line so it's not so insane looking :)
    std::string toString(int32_t forceDimensionId) {
      char tmpstring[1025];
        
      std::string s = "[";
      if ( playerLocalFlag || playerRemoteFlag ) {
        s += "Player";
      } else {
        s += "Mob";
      }

      if ( has_key(entityInfoList, idShort) ) {
        s += " Name=" + entityInfoList[idShort]->name;
      } else {
        sprintf(tmpstring," Name=(UNKNOWN: id=%d 0x%x)",idShort,idShort);
        s += tmpstring;
      }

      int32_t actualDimensionId = forceDimensionId;
      if ( dimensionId >= 0 ) {
        actualDimensionId = dimensionId;
        // getting dimension name from myWorld is more trouble than it's worth here :)
        sprintf(tmpstring," Dimension=%d", dimensionId);
        s += tmpstring;
      }

      // hack for pre-0.12 worlds
      if ( actualDimensionId < 0 ) {
        actualDimensionId = 0;
      }
        
      s += " Pos=" + pos.toStringWithImageCoords(actualDimensionId);
      s += " Rotation=(" + rotation.toString() + ")";

      if ( playerLocalFlag ) {

        // output player position + rotation to user
        // todo - option to put icon on map

        worldPointToGeoJSONPoint(actualDimensionId, pos.x,pos.z, playerPositionImageX, playerPositionImageY);
        playerPositionDimensionId = actualDimensionId;
        
        fprintf(stderr,"Player Position: Dimension=%d Pos=%s Rotation=(%lf, %lf)\n", actualDimensionId, pos.toStringWithImageCoords(actualDimensionId).c_str(), rotation.x,rotation.y);
      }

      if ( playerLocalFlag || playerRemoteFlag ) {
        // these are always in the overworld
        s += " BedPos=" + bedPosition.toStringWithImageCoords(kDimIdOverworld);
        s += " Spawn=" + spawn.toStringWithImageCoords(kDimIdOverworld);
      }

      if ( armorList.size() > 0 ) {
        s += " [Armor:";
        for ( const auto& it: armorList ) {
          std::string sarmor = it->toString(true,0);
          if ( sarmor.size() > 0 ) {
            s += " " + sarmor;
          }
        }
        s += "]";
      }

      if ( inventory.size() > 0 ) {
        s += " [Inventory:";
        for ( const auto& it: inventory ) {
          std::string sitem = it->toString(true,0);
          if ( sitem.size() > 0 ) {
            s += " " + sitem;
          }
        }
        s += "]";
      }

      if ( enderchest.size() > 0 ) {
        s += " [EnderChest:";
        for ( const auto& it: enderchest ) {
          std::string sitem = it->toString(true,0);
          if ( sitem.size() > 0 ) {
            s += " " + sitem;
          }
        }
        s += "]";
      }
      
      if ( itemInHand.valid ) {
        s += " ItemInHand=" + itemInHand.toString();
      }

      if ( item.valid ) {
        s += " Item=" + item.toString();
      }

      if ( ! otherPropsSortedFlag ) {
        std::sort( otherProps.begin(), otherProps.end() );
        otherPropsSortedFlag = true;
      }
      for ( const auto& it : otherProps ) {
        s += " " + it.first + "=" + it.second;
      }
        
      if ( tileId >= 0 ) {
        sprintf(tmpstring," Tile=[%s (%d 0x%x)]", blockInfoList[tileId].name.c_str(), tileId, tileId);
        s += tmpstring;
      }

      // todo - falling block also has "Data" (byte)

      // todo - dropped item also has "Fire" (short); "Health" (short)
        
      s += "]";
      return s;
    }
  };
  typedef std::vector< std::unique_ptr<ParsedEntity> > ParsedEntityList;



  class ParsedTileEntity {
  public:
    Point3d<double> pos;
    Point2d<int32_t> pairChest;
    std::string id;
    std::vector< std::unique_ptr<ParsedItem> > items;
    std::vector< std::string > text;
    std::vector< std::string > notes;
    int32_t signTotalStringLength;
    int32_t entityId;
    bool containerFlag;
    
    ParsedTileEntity() {
      clear();
    }
    void clear() {
      pos.clear();
      pairChest.clear();
      id = "";
      entityId = -1;
      items.clear();
      text.clear();
      notes.clear();
      signTotalStringLength = 0;
      containerFlag = false;
    }
    int32_t addItem ( nbt::tag_compound &iitem ) {
      std::unique_ptr<ParsedItem> it(new ParsedItem());
      int32_t ret = it->parse(iitem);
      items.push_back( std::move(it) );
      return ret;
    }
    int32_t addNote ( const std::string &s ) {
      notes.push_back( s );
      return 0;
    }
    int32_t addSign ( nbt::tag_compound &tc ) {
      text.push_back( tc["Text1"].as<nbt::tag_string>().get() );
      text.push_back( tc["Text2"].as<nbt::tag_string>().get() );
      text.push_back( tc["Text3"].as<nbt::tag_string>().get() );
      text.push_back( tc["Text4"].as<nbt::tag_string>().get() );
      signTotalStringLength=0;
      for ( const auto& it : text ) {
        // todo this should trim leading/trailing whitespace
        signTotalStringLength += it.length();
      }
      return 0;
    }
    int32_t addMobSpawner ( nbt::tag_compound &tc ) {
      entityId = tc["EntityId"].as<nbt::tag_int>().get();
      // todo - how to interpret entityId? (e.g. 0xb22 -- 0x22 is skeleton, what is 0xb?)
      // todo - any of these interesting?
      /*
        0x31-te: [] COMPOUND-1 {
        0x31-te:   [Delay] 20 0x14 (short)
        0x31-te:   [EntityId] 2850 0xb22 (int)
        0x31-te:   [MaxNearbyEntities] 6 0x6 (short)
        0x31-te:   [MaxSpawnDelay] 200 0xc8 (short)
        0x31-te:   [MinSpawnDelay] 200 0xc8 (short)
        0x31-te:   [RequiredPlayerRange] 16 0x10 (short)
        0x31-te:   [SpawnCount] 4 0x4 (short)
        0x31-te:   [SpawnRange] 4 0x4 (short)
      */
      return 0;
    }
    std::string toGeoJSON(int32_t forceDimensionId) {
      std::vector<std::string> list;
      char tmpstring[1025];

      if ( containerFlag ) {
        list.push_back("\"Name\":\"" + id + "\"");
          
        if ( pairChest.valid ) {
          // todobig - should we keep lists and combine chests so that we can show full content of double chests?
          list.push_back("\"pairchest\":[" + pairChest.toGeoJSON() + "]");
        }
          
        std::vector<std::string> tlist;
        for ( const auto& it: items ) {
          std::string sitem = it->toGeoJSON(true,0,true);
          if ( sitem.size() > 0 ) {
            tlist.push_back( sitem );
          }
        }
        if ( tlist.size() > 0 ) {
          std::string ts = "\"Items\":[";
          int32_t i = tlist.size();
          for (const auto& iter : tlist ) {
            ts += "{" + iter + "}";
            if ( --i > 0 ) {
              ts += ",";
            }
          }
          ts += "]";
          list.push_back(ts);
        }
      }

      if ( notes.size() > 0 ) {
        std::string ts = "\"Notes\":{";
        int32_t i = notes.size();
        int32_t t=1;
        for ( const auto& it: notes ) {
          sprintf(tmpstring,"\"Note%d\":\"%s\"", t++, escapeString(it,"\"\\").c_str());
          ts += tmpstring;
          if ( --i > 0 ) {
            ts += ",";
          }
        }
        ts += "}";
        list.push_back(ts);
      }
      
      if ( text.size() > 0 ) {
        std::string xname;
        if (signTotalStringLength <= 0) {
          xname = "SignBlank";
        } else {
          xname = "SignNonBlank";
        }

        list.push_back("\"Name\":\"" + xname + "\"");
        std::string ts = "\"" + xname + "\":{";

        int32_t i = text.size();
        int32_t t=1;
        for ( const auto& it: text ) {
          // todo - think about how to handle weird chars people put in signs
          sprintf(tmpstring,"\"Text%d\":\"%s\"", t++, escapeString(it,"\"\\").c_str());
          ts += tmpstring;
          if ( --i > 0 ) {
            ts += ",";
          }
        }
        ts += "}";
        list.push_back(ts);
      }

      if ( entityId > 0 ) {
        list.push_back("\"Name\":\"MobSpawner\"");
        std::string ts = "\"MobSpawner\":{";
        sprintf(tmpstring, "\"entityId\":\"%d (0x%x)\",", entityId, entityId);
        ts += tmpstring;
          
        // todo - the entityid is weird.  lsb appears to be entity type; high bytes are ??
        int32_t eid = entityId & 0xff;
        if ( has_key(entityInfoList, eid) ) {
          ts += "\"Name\":\"" + entityInfoList[eid]->name + "\"";
        } else {
          sprintf(tmpstring,"\"Name\":\"(UNKNOWN: id=%d 0x%x)\"",eid,eid);
          ts += tmpstring;
        }
        ts += "}";
        list.push_back(ts);
      }

      if ( list.size() > 0 ) {
        std::string s="";

        list.push_back(std::string("\"TileEntity\":\"true\""));
        sprintf(tmpstring,"\"Dimension\":\"%d\"", forceDimensionId);
        list.push_back(std::string(tmpstring));

        sprintf(tmpstring, "\"Pos\":[%s]", pos.toGeoJSON().c_str());
        list.push_back(std::string(tmpstring));
          
        double ix, iy;
        worldPointToGeoJSONPoint(forceDimensionId, pos.x,pos.z, ix,iy);
        s += makeGeojsonHeader(ix,iy);
          
        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        s += "}}";
        return s;
      }

      return std::string("");
    }
      
    // todo - this should probably be multi-line so it's not so insane looking :)
    std::string toString(int32_t dimensionId) {
      char tmpstring[1025];
        
      std::string s = "[";
        
      s += "Pos=" + pos.toStringWithImageCoords(dimensionId);

      if ( containerFlag ) {
        if ( pairChest.valid ) {
          // todobig - should we keep lists and combine chests so that we can show full content of double chests?
          s += " PairChest=(" + pairChest.toString() + ")";
        }
          
        s+=" " + id + "=[";
        int32_t i = items.size();
        for ( const auto& it: items ) {
          std::string sitem = it->toString(true,0);
          --i;
          if ( sitem.size() > 0 ) {
            s += sitem;
            if ( i > 0 ) {
              s += " ";
            }
          }
        }
        s += "]";
      }

      if ( text.size() > 0 ) {
        s+=" Sign=[";
        int32_t i = text.size();
        for ( const auto& it: text ) {
          s += it;
          if ( --i > 0 ) {
            s += " / ";
          }
        }
        s += "]";
      }

      if ( entityId > 0 ) {
        s+= " MobSpawner=[";
        // todo - the entityid is weird.  lsb appears to be entity type; high bytes are ??
        int32_t eid = entityId & 0xff;
        if ( has_key(entityInfoList, eid) ) {
          s += "Name=" + entityInfoList[eid]->name;
        } else {
          sprintf(tmpstring,"Name=(UNKNOWN: id=%d 0x%x)",eid,eid);
          s += tmpstring;
        }
        s += "]";
      }
        
      s += "]";
      return s;
    }
  };
  typedef std::vector< std::unique_ptr<ParsedTileEntity> > ParsedTileEntityList;

    
  int32_t parseNbt_entity(int32_t dimensionId, const std::string& dimName, MyNbtTagList &tagList,
                          bool playerLocalFlag, bool playerRemoteFlag,
                          const std::string &playerType, const std::string &playerId) {
    ParsedEntityList entityList;
    entityList.clear();

    int32_t actualDimensionId = dimensionId;
      
    // this could be a list of mobs
    for ( size_t i=0; i < tagList.size(); i++ ) { 

      std::unique_ptr<ParsedEntity> entity(new ParsedEntity());
      entity->clear();
        
      // check tagList
      if ( tagList[i].second->get_type() == nbt::tag_type::Compound ) {
        // all is good
      } else {
        fprintf(stderr,"ERROR: parseNbt_entity() called with invalid tagList (loop=%d)\n",(int)i);
        return -1;
      }
        
      nbt::tag_compound tc = tagList[i].second->as<nbt::tag_compound>();
        
      entity->playerLocalFlag = playerLocalFlag;
      entity->playerRemoteFlag = playerRemoteFlag;
      entity->playerType = playerType;
      entity->playerId = playerId;
        
      if ( tc.has_key("Armor", nbt::tag_type::List) ) {
        nbt::tag_list armorList = tc["Armor"].as<nbt::tag_list>();
        for ( const auto& iter: armorList ) {
          nbt::tag_compound armor = iter.as<nbt::tag_compound>();
          entity->addArmor(armor);
        }
      }
        
      if ( tc.has_key("Attributes") ) {
        // todo - parse - quite messy; interesting?
      }

      if ( playerLocalFlag || playerRemoteFlag ) {

        if ( tc.has_key("BedPositionX", nbt::tag_type::Int) ) {
          entity->bedPosition.set( tc["BedPositionX"].as<nbt::tag_int>().get(),
                                   tc["BedPositionY"].as<nbt::tag_int>().get(),
                                   tc["BedPositionZ"].as<nbt::tag_int>().get() );
        }

        if ( tc.has_key("SpawnX", nbt::tag_type::Int) ) {
          entity->spawn.set( tc["SpawnX"].as<nbt::tag_int>().get(),
                             tc["SpawnY"].as<nbt::tag_int>().get(),
                             tc["SpawnZ"].as<nbt::tag_int>().get() );
        }
      
        if ( tc.has_key("DimensionId", nbt::tag_type::Int) ) {
          entity->dimensionId = tc["DimensionId"].as<nbt::tag_int>().get();
          actualDimensionId = entity->dimensionId;
        }
      
        if ( tc.has_key("Inventory", nbt::tag_type::List) ) {
          nbt::tag_list inventory = tc["Inventory"].as<nbt::tag_list>();
          for ( const auto& iter: inventory ) {
            nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
            entity->addInventoryItem(iitem);
          }
        }

        if ( tc.has_key("EnderChestInventory", nbt::tag_type::List) ) {
          nbt::tag_list enderchest = tc["EnderChestInventory"].as<nbt::tag_list>();
          for ( const auto& iter: enderchest ) {
            nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
            entity->addEnderChestItem(iitem);
          }
        }
      } else {

        // non-player entity

        // todo - other interesting bits?
        
        if ( tc.has_key("ItemInHand", nbt::tag_type::Compound) ) {
          entity->doItemInHand( tc["ItemInHand"].as<nbt::tag_compound>() );
        }
        if ( tc.has_key("Item", nbt::tag_type::Compound) ) {
          entity->doItem( tc["Item"].as<nbt::tag_compound>() );
        }
        if ( tc.has_key("Tile", nbt::tag_type::Byte) ) {
          entity->doTile( tc["Tile"].as<nbt::tag_byte>() );
        }
        if ( tc.has_key("Items", nbt::tag_type::List) ) {
          nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
          for ( const auto& iter: items ) {
            nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
            entity->addInventoryItem(iitem);
          }
        }
      }

      if ( tc.has_key("Pos", nbt::tag_type::List) ) {
        nbt::tag_list pos = tc["Pos"].as<nbt::tag_list>();
        // todostopper -- crash here on xmas world -- bad cast -- pos not float?!
        entity->pos.set( pos[0].as<nbt::tag_float>().get(),
                         pos[1].as<nbt::tag_float>().get(),
                         pos[2].as<nbt::tag_float>().get() );
      }

      if ( tc.has_key("Rotation", nbt::tag_type::List) ) {
        nbt::tag_list rotation = tc["Rotation"].as<nbt::tag_list>();
        entity->rotation.set( rotation[0].as<nbt::tag_float>().get(),
                              rotation[1].as<nbt::tag_float>().get() );
      }
      
      if ( tc.has_key("id", nbt::tag_type::Int) ) {
        // in v0.16 entity id's went weird -- adding extra digits above 0xff -- could be flags of some sort?
        // todonow - let's figure out what these are - keep a list and print a summary at end
        entity->idFull = tc["id"].as<nbt::tag_int>().get();
        entity->idShort = entity->idFull & 0xFF;
      }

      // todo - diff entities have other fields:
      // see: http://minecraft.gamepedia.com/Chunk_format#Mobs

      // chicken: IsChickenJockey
      entity->checkOtherProp(tc, "IsChickenJockey");
        
      // ocelot: CatType
      entity->checkOtherProp(tc, "CatType");
        
      // sheep: Sheared; Color
      entity->checkOtherProp(tc, "Sheared");
      entity->checkOtherProp(tc, "Color");
        
      // skeleton: SkeletonType (wither skeleton!)
      entity->checkOtherProp(tc, "SkeletonType");

      // slime: Size
      entity->checkOtherProp(tc, "Size");
      entity->checkOtherProp(tc, "OnGround");
        
      // wolf: Angry; CollarColor
      entity->checkOtherProp(tc, "Angry");
      entity->checkOtherProp(tc, "CollarColor");
      entity->checkOtherProp(tc, "Owner"); 
      entity->checkOtherProp(tc, "OwnerNew"); // mcpe only?

      // villager: Profession; Riches; Career; CareerLevel; Willing; (Inventory); [Offers]
      entity->checkOtherProp(tc, "Profession");
      entity->checkOtherProp(tc, "Riches");
      entity->checkOtherProp(tc, "Career");
      entity->checkOtherProp(tc, "CareerLevel");
      entity->checkOtherProp(tc, "Willing");
      entity->checkOtherProp(tc, "Sitting");

      // make a summary field for villagers
      if ( true ) {
        // profession int
        // career int
        std::string vKey;
        int32_t vProfession = -1;
        int32_t vCareer = -1;

        vKey="Profession";
        if ( tc.has_key(vKey)) {
          vProfession = tc[vKey].as<nbt::tag_int>().get();
        }

        vKey="Career";
        if ( tc.has_key(vKey)) {
          vCareer = tc[vKey].as<nbt::tag_int>().get();
        }

        if ( vProfession >= 0 ) {
          std::string vValue = "";
          // values from: http://minecraft.gamepedia.com/Villager#Data_values
          switch (vProfession) {
          case 0:
            vValue += "Farmer";
            switch (vCareer) {
            case 1: vValue += ": Farmer"; break;
            case 2: vValue += ": Fisherman"; break;
            case 3: vValue += ": Shepherd"; break;
            case 4: vValue += ": Fletcher"; break;
            }
            break;
          case 1:
            vValue += "Librarian";
            break;
          case 2:
            vValue += "Priest";
            break;
          case 3:
            vValue += "Blacksmith";
            switch (vCareer) {
            case 1: vValue += ": Armorer"; break;
            case 2: vValue += ": Weapon Smith"; break;
            case 3: vValue += ": Tool Smith"; break;
            }
            break;
          case 4:
            vValue += "Butcher";
            switch (vCareer) {
            case 1: vValue += ": Butcher"; break;
            case 2: vValue += ": Leatherworker"; break;
            }
            break;
          default:
            vValue += "(Unknown VillagerType)";
            break;
          }
          entity->addOtherProp( "VillagerType", vValue );
        }
      }
      
     
      // horse: Bred, CanPickUpLoot, ChestedHorse, CUrrentHealth, EatingHaystack, Fire, HasCustomName, HasReproduced, IsGlobal, LeasherID, MaxHealth, OwnerNew, Saddled, Sitting, Temper, Type, Variant
      // todobig - other interesting stuff: armor (list), attributes (list), Items (list)
      entity->checkOtherProp(tc, "Bred");
      entity->checkOtherProp(tc, "CanPickUpLoot");
      entity->checkOtherProp(tc, "ChestedHorse");
      entity->checkOtherProp(tc, "CurrentHealth");
      entity->checkOtherProp(tc, "DeathTime");
      entity->checkOtherProp(tc, "EatingHaystack");
      entity->checkOtherProp(tc, "ForcedAge");
      entity->checkOtherProp(tc, "HasCustomName");
      entity->checkOtherProp(tc, "HasReproduced");
      entity->checkOtherProp(tc, "IsGlobal");
      entity->checkOtherProp(tc, "LeasherID");
      entity->checkOtherProp(tc, "MaxHealth");
      entity->checkOtherProp(tc, "Saddled");
      entity->checkOtherProp(tc, "Temper");
      entity->checkOtherProp(tc, "Type");
      entity->checkOtherProp(tc, "UniqueID");
      entity->checkOtherProp(tc, "Variant");

      // todo these will not work until we support list and compound in checkOtherProp
      //entity->checkOtherProp(tc, "Attributes");
      //entity->checkOtherProp(tc, "Armor");
      //entity->checkOtherProp(tc, "Items");

      // parse horse variant/type into a user-friendly string
      
      if ( true ) {
        std::string vKey;
        int32_t vType = -1;
        int32_t vVariant = -1;

        vKey="Variant";
        if ( tc.has_key(vKey)) {
          vVariant = tc[vKey].as<nbt::tag_int>().get();
        }

        vKey="Type";
        if ( tc.has_key(vKey)) {
          vType = tc[vKey].as<nbt::tag_int>().get();
        }

        if ( vType == 0 ) {
          // we only parse variants for horses

          std::string vbase="";
          switch (vVariant & 0xf) {
          case 0: vbase = "White"; break;
          case 1: vbase = "Creamy"; break;
          case 2: vbase = "Chestnut"; break;
          case 3: vbase = "Brown"; break;
          case 4: vbase = "Black"; break;
          case 5: vbase = "Gray"; break;
          case 6: vbase = "Dark Brown"; break;
          }

          std::vector<std::string> vadd;
          if (vVariant & 1024) {
            vadd.push_back("Black Dots");
          }
          else if (vVariant & 768) {
            vadd.push_back("White Dots");
          }
          else if (vVariant & 512) {
            vadd.push_back("White Field");
          }
          else if (vVariant & 256) {
            vadd.push_back("White");
          }

          std::string vout = "";
          if (vadd.size() > 0 ) {
            vout = vbase + " (";
            int32_t j = vadd.size();
            for (const auto& iter : vadd ) {
              vout += iter;
              if ( --j > 0 ) {
                vout += ", ";
              }
            }
            vout += ")";
          } else {
            vout = vbase;
          }

          entity->addOtherProp("HorseVariant", vout);

        }
      }
      

      // breedable mobs
      entity->checkOtherProp(tc, "InLove");
      entity->checkOtherProp(tc, "Age");
      entity->checkOtherProp(tc, "ForcedAge");
        
      // zombie: (IsVillager?); IsBaby;
      entity->checkOtherProp(tc, "IsBaby");

      // zombie pigman
      entity->checkOtherProp(tc, "Anger");
      entity->checkOtherProp(tc, "HurtBy");

      // enderman
      entity->checkOtherProp(tc, "carried");
      entity->checkOtherProp(tc, "carriedData");

      // creeper
      entity->checkOtherProp(tc, "IsPowered");
        
      // common
      entity->checkOtherProp(tc, "Health");
      entity->checkOtherProp(tc, "OwnerID"); 
      entity->checkOtherProp(tc, "Persistent");

      entity->checkOtherProp(tc, "PlayerCreated"); // golems?
        
      // IsGlobal - but appears to be always 0

      // todo - LinksTag - might be spider jockeys?
        
      // stuff I found:
      entity->checkOtherProp(tc, "SpawnedByNight");
        
      logger.msg(kLogInfo1, "%sParsedEntity: %s\n", dimName.c_str(), entity->toString(actualDimensionId).c_str());

      std::string geojson = entity->toGeoJSON(actualDimensionId);
      if ( geojson.length() > 0 ) {
        listGeoJSON.push_back( geojson );
      }

      entityList.push_back( std::move(entity) );
    }

    return 0;
  }

    
  int32_t parseNbt_tileEntity(int32_t dimensionId, const std::string& dimName, MyNbtTagList &tagList) {
    ParsedTileEntityList tileEntityList;
    tileEntityList.clear();
      
    // this could be a list of mobs
    for ( size_t i=0; i < tagList.size(); i++ ) { 

      bool parseFlag = false;
        
      std::unique_ptr<ParsedTileEntity> tileEntity(new ParsedTileEntity());
      tileEntity->clear();
        
      // check tagList
      if ( tagList[i].second->get_type() == nbt::tag_type::Compound ) {
        // all is good
      } else {
        fprintf(stderr,"ERROR: parseNbt_tileEntity() called with invalid tagList (loop=%d)\n",(int)i);
        return -1;
      }
        
      nbt::tag_compound tc = tagList[i].second->as<nbt::tag_compound>();

      if ( tc.has_key("x", nbt::tag_type::Int) ) {
        tileEntity->pos.set( tc["x"].as<nbt::tag_int>().get(),
                             tc["y"].as<nbt::tag_int>().get(),
                             tc["z"].as<nbt::tag_int>().get() );
      }

      if ( tc.has_key("pairx", nbt::tag_type::Int) ) {
        tileEntity->pairChest.set( tc["pairx"].as<nbt::tag_int>().get(),
                                   tc["pairz"].as<nbt::tag_int>().get() );
      }
        
      if ( tc.has_key("id", nbt::tag_type::String) ) {
        tileEntity->id = tc["id"].as<nbt::tag_string>().get();

        if ( false ) {
        }
        else if ( tileEntity->id == "Sign" ) {
          tileEntity->addSign(tc);
          parseFlag = true;
        }
        else if ( tileEntity->id == "Chest" ) {
          if ( tc.has_key("Items", nbt::tag_type::List) ) {
            tileEntity->containerFlag = true;
            nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
            for ( const auto& iter: items ) {
              nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
              tileEntity->addItem(iitem);
            }
            parseFlag = true;
          }
        }
        else if ( tileEntity->id == "EnderChest" ) {
          if ( tc.has_key("Items", nbt::tag_type::List) ) {
            tileEntity->containerFlag = true;
            nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
            for ( const auto& iter: items ) {
              nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
              tileEntity->addItem(iitem);
            }
            tileEntity->addNote("Ender Chest contents are only listed in Player entities (click Player in 'Passive Mobs')");
            parseFlag = true;
          }
          // todonow - other nbt tags: "id"; "isMovable"; "x","y","z"
        }
        else if ( tileEntity->id == "BrewingStand" ) {
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "EnchantTable" ) {
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "Furnace" ) {
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "MobSpawner" ) {
          tileEntity->addMobSpawner(tc);
          parseFlag = true;
        }
        else if ( tileEntity->id == "DaylightDetector" ) {
          // todo - new for 0.13
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "Skull" ) {
          // todo - new for 0.13
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "Music" ) {
          // todo - new for 0.13
          // todo - anything interesting?
        }
        else if ( tileEntity->id == "FlowerPot" ) {
          // todo - new for 0.13
          // todo - anything interesting?
          // todo - 'item' (short) is the blockid in the flower pot
          // todo - 'mData' (int) is the blockdata of the block in the pot? (e.g. flower / blue orchid)
        }
        else if ( tileEntity->id == "Hopper" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - has a LIST of items in the hopper; 'TransferCooldown'
          if ( tc.has_key("Items", nbt::tag_type::List) ) {
            tileEntity->containerFlag = true;
            nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
            for ( const auto& iter: items ) {
              nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
              tileEntity->addItem(iitem);
            }
            parseFlag = true;
          }
        }
        else if ( tileEntity->id == "Dropper" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - has a LIST of items in the dropper
          if ( tc.has_key("Items", nbt::tag_type::List) ) {
            tileEntity->containerFlag = true;
            nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
            for ( const auto& iter: items ) {
              nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
              tileEntity->addItem(iitem);
            }
            parseFlag = true;
          }
        }
        else if ( tileEntity->id == "Dispenser" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - has a LIST of items in the dispenser
          if ( tc.has_key("Items", nbt::tag_type::List) ) {
            tileEntity->containerFlag = true;
            nbt::tag_list items = tc["Items"].as<nbt::tag_list>();
            for ( const auto& iter: items ) {
              nbt::tag_compound iitem = iter.as<nbt::tag_compound>();
              tileEntity->addItem(iitem);
            }
            parseFlag = true;
          }
        }
        else if ( tileEntity->id == "Cauldron" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - has a LIST of items in the cauldron?; PotionId; SplashPotion
        }
        else if ( tileEntity->id == "ItemFrame" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - Item; ItemDropChance; ItemRotation
        }
        else if ( tileEntity->id == "Comparator" ) {
          // todo - new for 0.14
          // todo - anything interesting?
          // todo - 'OutputSignal'
        }
        else if ( tileEntity->id == "PistonArm" ) {
          // todo - new for 0.15
          // todo - anything interesting?
          /*
            0x31-te: NBT Decode Start
            0x31-te: [] COMPOUND-1 {
            0x31-te:   [AttachedBlocks] LIST-1 {
            0x31-te:   } LIST-1
            0x31-te:   [BreakBlocks] LIST-2 {
            0x31-te:   } LIST-2
            0x31-te:   [LastProgress] 0.000000 (float)
            0x31-te:   [LastRedstoneStrength] 0 0x0 (byte)
            0x31-te:   [NewState] 0 0x0 (byte)
            0x31-te:   [Progress] 0.000000 (float)
            0x31-te:   [State] 0 0x0 (byte)
            0x31-te:   [Sticky] 0 0x0 (byte)
            0x31-te:   [id] 'PistonArm' (string)
            0x31-te:   [needsUpdate] 1 0x1 (byte)
            0x31-te:   [x] 138 0x8a (int)
            0x31-te:   [y] 4 0x4 (int)
            0x31-te:   [z] 3 0x3 (int)
            0x31-te: } COMPOUND-1
            0x31-te: NBT Decode End (1 tags)
          */
        }
        
        else {
          logger.msg(kLogInfo1,"WARNING: Unknown tileEntity id=(%s)\n", tileEntity->id.c_str());
        }
      }

      if ( parseFlag ) {
        logger.msg(kLogInfo1, "%sParsedTileEntity: %s\n", dimName.c_str(), tileEntity->toString(dimensionId).c_str());

        std::string json = tileEntity->toGeoJSON(dimensionId);
        if ( json.size() > 0 ) {
          listGeoJSON.push_back( json );
        }
          
        tileEntityList.push_back( std::move(tileEntity) );
      }
    }
      
    return 0;
  }



  class ParsedPortal {
  public:
    Point3d<int> pos;
    int32_t dimId;
    int32_t span;
    int32_t xa, za;
      
    ParsedPortal() {
      clear();
    }
    void clear() {
      pos.clear();
      dimId = -1;
      span = xa = za = 0;
    }
    int32_t set ( nbt::tag_compound &tc ) {
      dimId = tc["DimId"].as<nbt::tag_int>().get();
      span = tc["Span"].as<nbt::tag_byte>().get();
      int32_t tpx = tc["TpX"].as<nbt::tag_int>().get();
      int32_t tpy = tc["TpY"].as<nbt::tag_int>().get();
      int32_t tpz = tc["TpZ"].as<nbt::tag_int>().get();
      pos.set(tpx,tpy,tpz);
      xa = tc["Xa"].as<nbt::tag_byte>().get();
      za = tc["Za"].as<nbt::tag_byte>().get();
      return 0;
    }
    std::string toGeoJSON() {
      std::vector<std::string> list;
      char tmpstring[1025];

      // note: we fake this as a tile entity so that it is easy to deal with in js
      list.push_back(std::string("\"TileEntity\":\"true\""));

      list.push_back("\"Name\":\"NetherPortal\"");

      sprintf(tmpstring,"\"DimId\":\"%d\"", dimId);
      list.push_back(tmpstring);

      sprintf(tmpstring,"\"Span\":\"%d\"", span);
      list.push_back(tmpstring);

      sprintf(tmpstring,"\"Xa\":\"%d\"", xa);
      list.push_back(tmpstring);

      sprintf(tmpstring,"\"Za\":\"%d\"", za);
      list.push_back(tmpstring);
        
      if ( list.size() > 0 ) {
        std::string s="";

        sprintf(tmpstring,"\"Dimension\":\"%d\"", dimId);
        list.push_back(std::string(tmpstring));

        sprintf(tmpstring, "\"Pos\":[%s]", pos.toGeoJSON().c_str());
        list.push_back(std::string(tmpstring));
          
        double ix, iy;
        worldPointToGeoJSONPoint(dimId, pos.x,pos.z, ix,iy);
        s += makeGeojsonHeader(ix,iy);
          
        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        s += "}}";
        return s;
      }

      return std::string("");
    }
      
    // todo - this should probably be multi-line so it's not so insane looking :)
    std::string toString() {
      char tmpstring[1025];

      std::string s = "[";

      s += "Nether Portal";
        
      s += " Pos=" + pos.toStringWithImageCoords(dimId);
        
      sprintf(tmpstring," DimId=%d", dimId);
      s += tmpstring;

      sprintf(tmpstring," Span=%d", span);
      s += tmpstring;

      sprintf(tmpstring," Xa=%d", xa);
      s += tmpstring;

      sprintf(tmpstring," Za=%d", za);
      s += tmpstring;
        
      s += "]";
      return s;
    }
  };
  typedef std::vector< std::unique_ptr<ParsedPortal> > ParsedPortalList;

    
  int32_t parseNbt_portals(MyNbtTagList &tagList) {
    ParsedPortalList portalList;
    portalList.clear();
      
    for ( size_t i=0; i < tagList.size(); i++ ) { 

      // check tagList
      if ( tagList[i].second->get_type() == nbt::tag_type::Compound ) {
        nbt::tag_compound tc = tagList[i].second->as<nbt::tag_compound>();
        if ( tc.has_key("data", nbt::tag_type::Compound) ) {
          nbt::tag_compound td = tc["data"].as<nbt::tag_compound>();
          if ( td.has_key("PortalRecords", nbt::tag_type::List) ) {
            // all is good
            nbt::tag_list plist = td["PortalRecords"].as<nbt::tag_list>();
              
            for ( const auto& it : plist ) {
              nbt::tag_compound pc = it.as<nbt::tag_compound>();

              std::unique_ptr<ParsedPortal> portal(new ParsedPortal());
              portal->clear();
                
              portal->set( pc );
                
              logger.msg(kLogInfo1, "ParsedPortal: %s\n", portal->toString().c_str());
                
              std::string json = portal->toGeoJSON();
              if ( json.size() > 0 ) {
                listGeoJSON.push_back( json );
              }
                
              portalList.push_back( std::move(portal) );
            }
          }
        }
      }
    }
      
    return 0;
  }


  class ParsedVillageDoor {
  public:
    int32_t idx, idz;
    int32_t ts;
    Point3d<int32_t> pos;
    
    ParsedVillageDoor() {
      clear();
    }
    void clear() {
      idx = idz = ts = 0;
      pos.clear();
    }
    int32_t set ( nbt::tag_compound &tc ) {
      idx = tc["IDX"].as<nbt::tag_int>().get();
      idz = tc["IDZ"].as<nbt::tag_int>().get();
      ts = tc["TS"].as<nbt::tag_int>().get();
      int32_t tx = tc["X"].as<nbt::tag_int>().get();
      int32_t ty = tc["Y"].as<nbt::tag_int>().get();
      int32_t tz = tc["Z"].as<nbt::tag_int>().get();
      pos.set(tx,ty,tz);
      return 0;
    }
    std::string toGeoJSON() {
      std::vector<std::string> list;
      char tmpstring[1025];

      sprintf(tmpstring, "\"idx\":%d", idx);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"idz\":%d", idz);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"ts\":%d", ts);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"Pos\":[%s]", pos.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));

      if ( list.size() > 0 ) {
        std::string s="";

        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        //s += "}}";
        return s;
      }

      return std::string("");
    }    
  };
  typedef std::vector< std::unique_ptr<ParsedVillageDoor> > ParsedVillageDoorList;


  class ParsedVillagePlayer {
  public:
    // todobig - don't know what this is yet
    ParsedVillagePlayer() {
      clear();
    }
    void clear() {
    }
    int32_t set ( nbt::tag_compound &tc ) {
      return 0;
    }
    std::string toGeoJSON() {
      return std::string("");
    }
  };
  typedef std::vector< std::unique_ptr<ParsedVillagePlayer> > ParsedVillagePlayerList;

  
  class ParsedVillageVillager {
  public:
    int64_t id;

    ParsedVillageVillager() {
      clear();
    }
    void clear() {
      id = 0;
    }
    int32_t set ( nbt::tag_compound &tc ) {
      id = tc["ID"].as<nbt::tag_long>().get();
      return 0;
    }
    std::string toGeoJSON() {
      std::vector<std::string> list;
      char tmpstring[1025];

      sprintf(tmpstring, "\"id\":%ld", id);
      list.push_back(std::string(tmpstring));

      if ( list.size() > 0 ) {
        std::string s="";

        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        //s += "}}";
        return s;
      }

      return std::string("");
    }    
  };
  typedef std::vector< std::unique_ptr<ParsedVillageVillager> > ParsedVillageVillagerList;

  
  class ParsedVillage {
  public:
    Point3d<int32_t> pos;
    Point3d<int32_t> apos; // todo - a fixed point pos? appears to be pos*48 + fractional part
    Point3d<double> fpos;  // calculated from apos

    ParsedVillageDoorList doorList;
    int32_t golems;
    int32_t mTick;
    ParsedVillagePlayerList playerList;
    int32_t radius;
    int32_t stable;
    int32_t tick;
    ParsedVillageVillagerList villagerList;
    
    ParsedVillage() {
      clear();
    }
    void clear() {
      pos.clear();
      apos.clear();
      fpos.clear();
      doorList.clear();
      golems = 0;
      mTick = 0;
      playerList.clear();
      radius = 0;
      stable = 0;
      tick = 0;
      villagerList.clear();
    }
    int32_t set ( nbt::tag_compound &tc ) {
      int32_t acx = tc["ACX"].as<nbt::tag_int>().get();
      int32_t acy = tc["ACY"].as<nbt::tag_int>().get();
      int32_t acz = tc["ACZ"].as<nbt::tag_int>().get();
      apos.set(acx,acy,acz);
      // todobig - correct this to a true floating point number?
      
      int32_t cx = tc["CX"].as<nbt::tag_int>().get();
      int32_t cy = tc["CY"].as<nbt::tag_int>().get();
      int32_t cz = tc["CZ"].as<nbt::tag_int>().get();
      pos.set(cx,cy,cz);

      fpos.set(acx / 48.0, acy / 48.0, acz / 48.0);
      
      if ( tc.has_key("Doors", nbt::tag_type::List) ) {
        nbt::tag_list dlist = tc["Doors"].as<nbt::tag_list>();
        
        for ( const auto& it : dlist ) {
          nbt::tag_compound dc = it.as<nbt::tag_compound>();
          std::unique_ptr<ParsedVillageDoor> door(new ParsedVillageDoor());
          door->set( dc );
          doorList.push_back( std::move(door) );
        }
      }

      golems = tc["Golems"].as<nbt::tag_int>().get();
      mTick = tc["MTick"].as<nbt::tag_int>().get();
          
      if ( tc.has_key("Players", nbt::tag_type::List) ) {
        nbt::tag_list dlist = tc["Players"].as<nbt::tag_list>();
        
        for ( const auto& it : dlist ) {
          nbt::tag_compound pc = it.as<nbt::tag_compound>();
          std::unique_ptr<ParsedVillagePlayer> player(new ParsedVillagePlayer());
          player->set( pc );
          playerList.push_back( std::move(player) );
        }
      }

      radius = tc["Radius"].as<nbt::tag_int>().get();
      stable = tc["Stable"].as<nbt::tag_int>().get();
      tick = tc["Tick"].as<nbt::tag_int>().get();
          
      if ( tc.has_key("Villagers", nbt::tag_type::List) ) {
        nbt::tag_list dlist = tc["Villagers"].as<nbt::tag_list>();
        
        for ( const auto& it : dlist ) {
          nbt::tag_compound vc = it.as<nbt::tag_compound>();
          std::unique_ptr<ParsedVillageVillager> villager(new ParsedVillageVillager());
          villager->set( vc );
          villagerList.push_back( std::move(villager) );
        }
      }

      return 0;
    }
    std::string toGeoJSON() {
      std::vector<std::string> list;
      std::vector<std::string> templist;
      //todobig - something better than this :)
      char tmpstring[8192];

      // note: we fake this as a tile entity so that it is easy to deal with in js
      list.push_back(std::string("\"TileEntity\":\"true\""));

      list.push_back("\"Name\":\"Village\"");

      sprintf(tmpstring, "\"Pos\":[%s]", pos.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"APos\":[%s]", apos.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"FPos\":[%s]", fpos.toGeoJSON().c_str());
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"golems\":%d", golems);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"mTick\":%d", mTick);
      list.push_back(std::string(tmpstring));

      sprintf(tmpstring, "\"radius\":%d", radius);
      list.push_back(std::string(tmpstring));
      
      sprintf(tmpstring, "\"stable\":%d", stable);
      list.push_back(std::string(tmpstring));
      
      sprintf(tmpstring, "\"tick\":%d", tick);
      list.push_back(std::string(tmpstring));
      
      templist.clear();
      for ( const auto& it : villagerList ) {
        templist.push_back( "{" + it->toGeoJSON() + "}" );
      }
      if ( templist.size() > 0 ) {
        std::string s="";
        int32_t i = templist.size();
        for (const auto& iter : templist ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        sprintf(tmpstring, "\"Villagers\":[%s]", s.c_str());
        list.push_back(std::string(tmpstring));
      } else {
        sprintf(tmpstring, "\"Villagers\":[]");
        list.push_back(std::string(tmpstring));
      }

      templist.clear();
      for ( const auto& it : doorList ) {
        templist.push_back( "{" + it->toGeoJSON() + "}" );
      }
      if ( templist.size() > 0 ) {
        std::string s="";
        int32_t i = templist.size();
        for (const auto& iter : templist ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        //sprintf(tmpstring, "\"Doors\":[%s]", s.c_str());
        //list.push_back(std::string(tmpstring));
        std::string sout = "\"Doors\":[" + s + "]";
        list.push_back(sout);
      } else {
        sprintf(tmpstring, "\"Doors\":[]");
        list.push_back(std::string(tmpstring));
      }
      
      templist.clear();
      for ( const auto& it : playerList ) {
        templist.push_back( "{" + it->toGeoJSON() + "}" );
      }
      if ( templist.size() > 0 ) {
        std::string s="";
        int32_t i = templist.size();
        for (const auto& iter : templist ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        sprintf(tmpstring, "\"Players\":[%s]", s.c_str());
        list.push_back(std::string(tmpstring));
      } else {
        sprintf(tmpstring, "\"Players\":[]");
        list.push_back(std::string(tmpstring));
      }

      if ( list.size() > 0 ) {
        std::string s="";

        sprintf(tmpstring,"\"Dimension\":\"%d\"", 0);
        list.push_back(std::string(tmpstring));

        // todobig - just a test
#if 1
        // point style
        double ix, iy;
        worldPointToGeoJSONPoint(0, fpos.x,fpos.z, ix,iy);
        s += makeGeojsonHeader(ix,iy,false);
#else
        // multi-point style
        int npoints = doorList.size() + 1;
        double *xlist = new double[ npoints ];
        double *ylist = new double[ npoints ];

        int p=0;
        double ix, iy;
        worldPointToGeoJSONPoint(0, fpos.x,fpos.z, ix,iy);
        xlist[p]=ix;
        ylist[p]=iy;
        p++;

        for ( const auto& it : doorList ) {
          worldPointToGeoJSONPoint(0, it->pos.x, it->pos.z, ix,iy);
          xlist[p]=ix;
          ylist[p]=iy;
          p++;
        }

        s += makeGeojsonHeader_MultiPoint(npoints, xlist, ylist);

        delete [] xlist;
        delete [] ylist;
#endif

        int32_t i = list.size();
        for (const auto& iter : list ) {
          s += iter;
          if ( --i > 0 ) {
            s += ",";
          }
        }
        s += "}}";
        return s;
      }

      return std::string("");
    }
      
    // todo - this should probably be multi-line so it's not so insane looking :)
    std::string toString() {
      std::string s = "";
      return s;
    }
  };
  typedef std::vector< std::unique_ptr<ParsedVillage> > ParsedVillageList;
  
  int32_t parseNbt_mVillages(MyNbtTagList &tagList) {
    ParsedVillageList villageList;
    villageList.clear();
      
    for ( size_t i=0; i < tagList.size(); i++ ) { 

      // check tagList
      if ( tagList[i].second->get_type() == nbt::tag_type::Compound ) {
        nbt::tag_compound tc = tagList[i].second->as<nbt::tag_compound>();
        if ( tc.has_key("data", nbt::tag_type::Compound) ) {
          nbt::tag_compound td = tc["data"].as<nbt::tag_compound>();

          int32_t vTick = td["Tick"].as<nbt::tag_int>().get();
          
          if ( td.has_key("Villages", nbt::tag_type::List) ) {
            // all is good
            nbt::tag_list vlist = td["Villages"].as<nbt::tag_list>();
              
            for ( const auto& it : vlist ) {
              nbt::tag_compound pc = it.as<nbt::tag_compound>();
          
              std::unique_ptr<ParsedVillage> village(new ParsedVillage());
              village->clear();
              village->set( pc );
              
              logger.msg(kLogInfo1, "ParsedVillage: %s\n", village->toString().c_str());
              
              std::string json = village->toGeoJSON();
              if ( json.size() > 0 ) {
                listGeoJSON.push_back( json );
              }
              
              villageList.push_back( std::move(village) );
            }
          }
        }
      }
    }
      
    return 0;
  }


  int32_t writeSchematicFile(const std::string& fn, int32_t sizex, int32_t sizey, int32_t sizez,
                             nbt::tag_byte_array& blockArray, nbt::tag_byte_array& blockDataArray) {

    std::ofstream os(fn, std::ofstream::out);
    // todozzz - error checking here (chmod?)

    nbt::io::stream_writer writer(os);
    // todozzz - zlib?

    // create schematic file nbt tag and populate it
    nbt::tag_compound tag;
    tag.emplace<nbt::tag_short>("Width", sizex);
    tag.emplace<nbt::tag_short>("Height", sizey);
    tag.emplace<nbt::tag_short>("Length", sizez);
    tag.emplace<nbt::tag_string>("Materials", "Alpha");
    tag.emplace<nbt::tag_byte_array>("Blocks", blockArray);
    tag.emplace<nbt::tag_byte_array>("Data", blockDataArray);
    // todozzz - entity list
    //tag.emplace<nbt::tag_list>("Entities", NULL);
    // todozzz - tileentity list
    //tag.emplace<nbt::tag_list>("TileEntities", NULL);

    // write the nbt file
    writer.write_tag("Schematic",tag);

    os.close();
    
    /*
      schematic file format
      from: http://minecraft.gamepedia.com/Schematic_file_formathttp://minecraft.gamepedia.com/Schematic_file_format
        
      COMPOUND Schematic: Schematic data.
      SHORT Width: Size along the X axis.
      SHORT Height: Size along the Y axis.
      SHORT Length: Size along the Z axis.
      STRING Materials: This will be "Classic" for schematics exported from Minecraft Classic levels, and "Alpha" for those from Minecraft Alpha and newer levels.
      BYTE_ARRAY Blocks: Block IDs defining the terrain. 8 bits per block. Sorted by height (bottom to top) then length then width -- the index of the block at X,Y,Z is (Y*length + Z)*width + X.
      BYTE_ARRAY Data: Block data additionally defining parts of the terrain. Only the lower 4 bits of each byte are used. (Unlike in the chunk format, the block data in the schematic format occupies a full byte per block.)
      LIST Entities: List of Compound tags.
      -- COMPOUND A single entity in the schematic.
      -- See the Chunk Format -> Entity Format.
      LIST TileEntities: List of Compound tags.
      -- COMPOUND A single tile entity in the schematic.
      -- See Chunk Format -> Tile Entity Format.
    */
    
    return 0;
  }
  
} // namespace mcpe_viz

/**
 * @file llviewertexturelist.h
 * @brief Object for managing the list of images within a region
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLVIEWERTEXTURELIST_H
#define LL_LLVIEWERTEXTURELIST_H

#include "lluuid.h"
//#include "message.h"
#include "llgl.h"
#include "llviewertexture.h"
#include "llui.h"
#include <list>
#include <unordered_set>
#include "lluiimage.h"

const U32 LL_IMAGE_REZ_LOSSLESS_CUTOFF = 128;

const bool MIPMAP_YES = true;
const bool MIPMAP_NO = false;

const bool GL_TEXTURE_YES = true;
const bool GL_TEXTURE_NO = false;

const bool IMMEDIATE_YES = true;
const bool IMMEDIATE_NO = false;

class LLImageJ2C;
class LLMessageSystem;
class LLTextureView;

typedef void (*LLImageCallback)(bool success,
                                LLViewerFetchedTexture *src_vi,
                                LLImageRaw* src,
                                LLImageRaw* src_aux,
                                S32 discard_level,
                                bool final,
                                void* userdata);

enum ETexListType
{
    TEX_LIST_STANDARD = 0,
    TEX_LIST_SCALE
};

// <FS:minerjr>
// Moved this here as we can re-use this structure for our new 
typedef std::vector<std::pair<FSTextureRequest, LLPointer<LLViewerFetchedTexture>>> entries_list_t;
typedef std::pair<FSTextureRequest, LLPointer<LLViewerFetchedTexture>> FSTextureRequestPair;
// New class to track all of the fun changes to the textures in the current frame, and to control which ones go through
class FSTextureRequestList
{
public:
    FSTextureRequestList();
    // Returns true if the request was accepted, false if rejected. Though the request could be rejected later on if higher priority texture appears
    //bool addRequest(FSTextureRequest newRequest, S32 textureIndex);

    // Clear the current texture requests, could make it so we hold on to the list and keep updating the
    // change requests
    bool clearRequests();

    // This will sort the list of texture changes
    bool processRequests(bool ignore_limits = false);

    bool applyRequest(S32 index);

    // Helper function to init a FSTextureRequest with the data form an image
    static FSTextureRequest initRequestFromTexture(LLViewerFetchedTexture* imagep);

    // Static declair of values
    const static S32 MAX_TEXTURE_REQUEST_SIZE = 512;
    //const static S32 NUMBER_OF_TEXTURE_REQUEST_LISTS = 2;

    // Fun bit mask flag time to reverse the values of the discard as 0 is higher quality
    const static U64 LOW_VRAM_DROP_INCREASES = 16140901047315988480;
    const static U64 LOW_VRAM_ALLOW_INCREASES = 18446744056529682432;
    const static U64 FLIP_DISCARD_TEXTURE_MASK = 2733761;

    // std::sort function for sorting the pair's of texture request and actual textures


    // This is for the scenario of low memory where we want to drop all texture increases
    static bool sort_low_memory_drop_increases(const FSTextureRequestPair& a, const FSTextureRequestPair& b)
    {
        // Simple look the raw value and rank based upon the current set hierarchy of values
        // with a mask which restricts that only decrease textures will have the higher sort value
        //static FSTextureRequest Mask;
        //Mask.Raw = 0;
        //Mask.TextureRequest.DecreasePixelChange = 1;
        //Mask.TextureRequest.NeedToDelete = 1;
        //Mask.TextureRequest.ScaleDown = 1;
        //Mask.TextureRequest.PixelChange = 67108863;

        return (a.first.Raw & LOW_VRAM_DROP_INCREASES) > (b.first.Raw & LOW_VRAM_DROP_INCREASES);
    }

    // This is for the scenario of low memory where we want to prioritize texture
    // decreases, but allow some increases
    static bool sort_low_memory_allow_increases(const FSTextureRequestPair& a, const FSTextureRequestPair& b)
    {
        // Simple look the raw value and rank based upon the current set hierarchy of values
        //static FSTextureRequest Mask;
        //Mask.Raw = 0;
        //Mask.TextureRequest.OnScreen = 1;
        //Mask.TextureRequest.DecreasePixelChange = 1;
        //Mask.TextureRequest.NeedToDelete = 1;
        //Mask.TextureRequest.ScaleDown = 1;
        //Mask.TextureRequest.PixelChange = 67108863;

        return (a.first.Raw & LOW_VRAM_ALLOW_INCREASES) > (b.first.Raw & LOW_VRAM_ALLOW_INCREASES);
    }
    
    // This is for the normal memory scenario
    static bool sort_normal_memory(const FSTextureRequestPair& a, const FSTextureRequestPair& b)
    {
        // Simple look the raw value and rank based upon the current set hierarchy of values
        // We have a make to simply invert the discard values go from (0 don't discard) to
        // (6 - MAX_DISCARD + 1 dicard everything)
        //static FSTextureRequest Mask;
        //Mask.Raw = 0;
        //Mask.TextureRequest.Type = LLViewerTexture::INVALID_TEXTURE_TYPE;
        //Mask.TextureRequest.NewDiscard = MAX_DISCARD_LEVEL + 1;    
        //Mask.TextureRequest.OldDiscard = MAX_DISCARD_LEVEL + 1;
        //Mask.TextureRequest.NewDesiredDiscard = MAX_DISCARD_LEVEL + 1;
        //Mask.TextureRequest.OldDesiredDiscard = MAX_DISCARD_LEVEL + 1;
        //Mask.TextureRequest.TextListType = 1;

        return (a.first.Raw) > (b.first.Raw);
        //return (~a.first.Raw & FLIP_DISCARD_TEXTURE_MASK) < (~b.first.Raw & FLIP_DISCARD_TEXTURE_MASK);
    }

    // Accessor methods for the Vector of Fetch Textures
    entries_list_t &getTexturesToModify() { return mTexturesToModify; }
    // Creates a pair of FSTextureRequest along with the texture
    bool addTextureToModify(LLPointer<LLViewerFetchedTexture> newTexture);

    FSTextureRequestPair &getLastPair() { return mTexturesToModify.back(); }

    bool updateStats(FSTextureRequest currentTextureRequest);

private:
    
    // Strcut to store the 2 texture request lists
    //FSTextureRequestList mTextureRequestLists[NUMBER_OF_TEXTURE_REQUEST_LISTS];
    // We now store the list of textures to modify in side here instead of creating a new list
    // every frame.
    entries_list_t mTexturesToModify;

    U64 mDecreaseRAMAmount;
    U64 mIncreaseRAMAmount;
    U64 mDecreaseVRAMAmount;
    U64 mIncreaseVRAMAMount;

    S32 mNumberOfDecreaseRequests;
    S32 mNumberOfIncreaseRequests;

};
// </FS:minerjr>

struct LLTextureKey
{
    LLTextureKey();
    LLTextureKey(LLUUID id, ETexListType tex_type);
    LLUUID textureId;
    ETexListType textureType;

    friend bool operator<(const LLTextureKey& key1, const LLTextureKey& key2)
    {
        if (key1.textureId != key2.textureId)
        {
            return key1.textureId < key2.textureId;
        }
        else
        {
            return key1.textureType < key2.textureType;
        }
    }
};

class LLViewerTextureList
{
    friend class LLTextureView;
    friend class LLViewerTextureManager;
    friend class LLLocalBitmap;

public:
    static bool createUploadFile(LLPointer<LLImageRaw> raw_image,
                                 const std::string& out_filename,
                                 const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                 const S32 min_image_dimentions = 0);
    static bool createUploadFile(const std::string& filename,
                                 const std::string& out_filename,
                                 const U8 codec,
                                 const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                 const S32 min_image_dimentions = 0,
                                 bool force_square = false);
    static LLPointer<LLImageJ2C> convertToUploadFile(LLPointer<LLImageRaw> raw_image,
                                                     const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                                     bool force_square = false,
                                                     bool force_lossless = false);
    static void processImageNotInDatabase( LLMessageSystem *msg, void **user_data );
    // <FS:Ansariel> OpenSim compatibility
    static void receiveImageHeader(LLMessageSystem *msg, void **user_data);
    static void receiveImagePacket(LLMessageSystem *msg, void **user_data);
    // </FS:Ansariel>

public:
    LLViewerTextureList();
    ~LLViewerTextureList();

    void init();
    void shutdown();
    void dump();
    void destroyGL();
    bool isInitialized() const {return mInitialized;}

    void findTexturesByID(const LLUUID &image_id, std::vector<LLViewerFetchedTexture*> &output);
    LLViewerFetchedTexture *findImage(const LLUUID &image_id, ETexListType tex_type);
    LLViewerFetchedTexture *findImage(const LLTextureKey &search_key);

    // Using image stats, determine what images are necessary, and perform image updates.
    void updateImages(F32 max_time);
    void forceImmediateUpdate(LLViewerFetchedTexture* imagep) ;

    // Decode and create textures for all images currently in list.
    void decodeAllImages(F32 max_decode_time);

    void handleIRCallback(void **data, const S32 number);

    S32 getNumImages()                  { return static_cast<S32>(mImageList.size()); }

    // Local UI images
    // Local UI images
    void doPreloadImages();
    // Network images. Needs caps and cache to work
    void doPrefetchImages();

    void clearFetchingRequests();

    // do some book keeping on the specified texture
    // - updates decode priority
    // - updates desired discard level
    // - cleans up textures that haven't been referenced in awhile
    // <FS:minerjr>
    //void updateImageDecodePriority(LLViewerFetchedTexture* imagep, bool flush_images = true);
    // The updated to return the state of the texture before and after the decode priority change
    void updateImageDecodePriority(FSTextureRequestPair &imagep , bool flush_images = true);
    std::shared_ptr<FSTextureRequestList> mTextureRequestList;
    // </FS:minerjr>

private:
    F32  updateImagesCreateTextures(F32 max_time);
    F32  updateImagesFetchTextures(F32 max_time);
    void updateImagesUpdateStats();
    F32  updateImagesLoadingFastCache(F32 max_time);

    void addImage(LLViewerFetchedTexture *image, ETexListType tex_type);
    void deleteImage(LLViewerFetchedTexture *image);

    void addImageToList(LLViewerFetchedTexture *image);
    void removeImageFromList(LLViewerFetchedTexture *image);

public:     // PoundLife - Improved Object Inspect
    LLViewerFetchedTexture * getImage(const LLUUID &image_id,
                                     FTType f_type = FTT_DEFAULT,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     LLHost request_from_host = LLHost()
                                     );

private:    // PoundLife - Improved Object Inspect
    LLViewerFetchedTexture * getImageFromFile(const std::string& filename,
                                     FTType f_type = FTT_LOCAL_FILE,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     const LLUUID& force_id = LLUUID::null
                                     );

    LLViewerFetchedTexture* getImageFromUrl(const std::string& url,
                                     FTType f_type,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                      LLGLenum primary_format = 0,
                                     const LLUUID& force_id = LLUUID::null
                                     );

    LLImageRaw* getRawImageFromMemory(const U8* data, U32 size, std::string_view mimetype);
    LLViewerFetchedTexture* getImageFromMemory(const U8* data, U32 size, std::string_view mimetype);

    LLViewerFetchedTexture* createImage(const LLUUID &image_id,
                                     FTType f_type,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     LLHost request_from_host = LLHost()
                                     );

    // Request image from a specific host, used for baked avatar textures.
    // Implemented in header in case someone changes default params above. JC
    LLViewerFetchedTexture* getImageFromHost(const LLUUID& image_id, FTType f_type, LLHost host)
    { return getImage(image_id, f_type, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, host); }

public:
    typedef std::unordered_set<LLPointer<LLViewerFetchedTexture> > image_list_t;
    typedef std::queue<LLPointer<LLViewerFetchedTexture> > image_queue_t;

    // images that have been loaded but are waiting to be uploaded to GL
    image_queue_t mCreateTextureList;

    // images that must be downscaled quickly so we don't run out of memory
    image_queue_t mDownScaleQueue;

    image_list_t mCallbackList;
    image_list_t mFastCacheList;

    bool mForceResetTextureStats;

    // to make "for (auto& imagep : gTextureList)" work
    const image_list_t::const_iterator begin() const { return mImageList.cbegin(); }
    const image_list_t::const_iterator end() const { return mImageList.cend(); }

    // <FS:Ansariel> Fast cache stats
    static U32 sNumFastCacheReads;

private:
    typedef std::map< LLTextureKey, LLPointer<LLViewerFetchedTexture> > uuid_map_t;
    uuid_map_t mUUIDMap;
    LLTextureKey mLastUpdateKey;

    image_list_t mImageList;

    // simply holds on to LLViewerFetchedTexture references to stop them from being purged too soon
    std::unordered_set<LLPointer<LLViewerFetchedTexture> > mImagePreloads;

    bool mInitialized ;
    LLFrameTimer mForceDecodeTimer;

private:
    static S32 sNumImages;
    static void (*sUUIDCallback)(void**, const LLUUID &);
    LOG_CLASS(LLViewerTextureList);
};

class LLUIImageList : public LLImageProviderInterface, public LLSingleton<LLUIImageList>
{
    LLSINGLETON_EMPTY_CTOR(LLUIImageList);
public:
    // LLImageProviderInterface
    /*virtual*/ LLPointer<LLUIImage> getUIImageByID(const LLUUID& id, S32 priority) override;
    /*virtual*/ LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority) override;
    void cleanUp() override;

    bool initFromFile();

    LLPointer<LLUIImage> preloadUIImage(const std::string& name, const std::string& filename, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle stype);

    static void onUIImageLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, bool final, void* userdata );
private:
    LLPointer<LLUIImage> loadUIImageByName(const std::string& name, const std::string& filename,
                                   bool use_mips = false, const LLRect& scale_rect = LLRect::null,
                                   const LLRect& clip_rect = LLRect::null,
                                   LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
                                   LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);
    LLPointer<LLUIImage> loadUIImageByID(const LLUUID& id,
                                 bool use_mips = false, const LLRect& scale_rect = LLRect::null,
                                 const LLRect& clip_rect = LLRect::null,
                                 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
                                 LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);

    LLPointer<LLUIImage> loadUIImage(LLViewerFetchedTexture* imagep, const std::string& name, bool use_mips = false, const LLRect& scale_rect = LLRect::null, const LLRect& clip_rect = LLRect::null, LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);


    struct LLUIImageLoadData
    {
        std::string mImageName;
        LLRect mImageScaleRegion;
        LLRect mImageClipRegion;
    };

    typedef std::map< std::string, LLPointer<LLUIImage> > uuid_ui_image_map_t;
    uuid_ui_image_map_t mUIImages;

    //
    //keep a copy of UI textures to prevent them to be deleted.
    //mGLTexturep of each UI texture equals to some LLUIImage.mImage.
    std::list< LLPointer<LLViewerFetchedTexture> > mUITextureList ;
};

const bool GLTEXTURE_TRUE = true;
const bool GLTEXTURE_FALSE = false;
const bool MIPMAP_TRUE = true;
const bool MIPMAP_FALSE = false;

extern LLViewerTextureList gTextureList;

#endif

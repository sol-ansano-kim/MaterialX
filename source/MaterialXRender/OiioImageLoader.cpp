//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXRender/OiioImageLoader.h>

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4100)
    #pragma warning(disable: 4244)
#endif

#include <OpenImageIO/imageio.h>

#if defined(_WIN32)
    #pragma warning(pop)
#endif

namespace MaterialX
{

bool OiioImageLoader::saveImage(const FilePath& filePath,
                                ConstImagePtr image,
                                bool verticalFlip)
{
    OIIO::ImageSpec imageSpec;
    imageSpec.width = image->getWidth();
    imageSpec.height = image->getHeight();
    imageSpec.nchannels = image->getChannelCount();

    OIIO::TypeDesc format;
    switch (image->getBaseType())
    {
        case Image::BaseType::UINT8:
            format = OIIO::TypeDesc::UINT8;
            break;
        case Image::BaseType::HALF:
            format = OIIO::TypeDesc::HALF;
            break;
        case Image::BaseType::FLOAT:
            format = OIIO::TypeDesc::FLOAT;
            break;
        default:
            return false;
    }

    bool written = false;
    std::unique_ptr<OIIO::ImageOutput> imageOutput = OIIO::ImageOutput::create(filePath);
    if (imageOutput)
    {
        if (imageOutput->open(filePath, imageSpec))
        {
            if (verticalFlip)
            {
                int scanlinesize = image->getWidth() * image->getChannelCount() * image->getBaseStride();
                written = imageOutput->write_image(
                    format,
                    static_cast<char*>(image->getResourceBuffer()) + (image->getHeight() - 1) * scanlinesize,
                    OIIO::AutoStride, // default x stride
                    static_cast<OIIO::stride_t>(-scanlinesize), // special y stride
                    OIIO::AutoStride);
            }
            else
            {
                written = imageOutput->write_image(
                    format,
                    image->getResourceBuffer());
            }
            imageOutput->close();
        }
    }
    return written;
}

ImagePtr OiioImageLoader::loadImage(const FilePath& filePath)
{
    std::unique_ptr<OIIO::ImageInput> imageInput = OIIO::ImageInput::open(filePath);
    if (!imageInput)
    {
        return nullptr;
    }

    OIIO::ImageSpec imageSpec = imageInput->spec();
    Image::BaseType baseType;
    switch (imageSpec.format.basetype)
    {
        case OIIO::TypeDesc::UINT8:
            baseType = Image::BaseType::UINT8;
            break;
        case OIIO::TypeDesc::HALF:
            baseType = Image::BaseType::HALF;
            break;
        case OIIO::TypeDesc::FLOAT:
            baseType = Image::BaseType::FLOAT;
            break;
        default:
            imageInput->close();
            return nullptr;
    };

    ImagePtr image = Image::create(imageSpec.width, imageSpec.height, imageSpec.nchannels, baseType);
    image->createResourceBuffer();
    if (!imageInput->read_image(imageSpec.format, image->getResourceBuffer()))
    {
        image = nullptr;
    }
    imageInput->close();

    return image;
}

} // namespace MaterialX
/*=========================================================================

 Program:   ORFEO Toolbox
 Language:  C++
 Date:      $Date$
 Version:   $Revision$


 Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
 See OTBCopyright.txt for details.


 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 =========================================================================*/
#include "otbWrapperApplication.h"
#include "otbWrapperApplicationFactory.h"

#include "otbPerBandVectorImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkShrinkImageFilter.h"

#include "otbWrapperParameter.h"
#include "otbWrapperOutputImageParameter.h"

namespace otb
{
namespace Wrapper
{

class MultiResolutionPyramid : public Application
{

public:
  /** Standard class typedefs. */
  typedef MultiResolutionPyramid        Self;
  typedef Application                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Standard macro */
  itkNewMacro(Self);

  itkTypeMacro(MultiResolutionPyramid, otb::Application);

  /** Image and filters typedef */
  typedef itk::DiscreteGaussianImageFilter<FloatImageType,
                                           FloatImageType>         SmoothingImageFilterType;

  typedef otb::PerBandVectorImageFilter<FloatVectorImageType,
                                        FloatVectorImageType,
                                        SmoothingImageFilterType>   SmoothingVectorImageFilterType;

  typedef itk::ShrinkImageFilter<FloatVectorImageType,
                                 FloatVectorImageType>              ShrinkFilterType;

private:
  MultiResolutionPyramid()
  {
    SetName("MultiResolutionPyramid");
    SetDescription("Build a multi-resolution pyramid of the image.");
  }

  void DoCreateParameters()
  {
    AddParameter(ParameterType_InputImage, "in", "Input Image");

    AddParameter(ParameterType_Filename, "out", "Output Image");
    SetParameterDescription("out","will be used to get the prefix and the extension of the images to write");
    SetParameterRole("out",Role_Output);

    AddParameter(ParameterType_Int, "level", "Number Of Levels");
    SetParameterInt("level", 1);
    SetParameterDescription( "level", "Number of levels in the pyramid (default is 1)");

    AddParameter(ParameterType_Int, "sfactor", "Subsampling factor");
    SetParameterInt("sfactor", 2);

    AddParameter(ParameterType_Float,  "vfactor", "Subsampling factor");
    SetParameterFloat("vfactor", 0.6);

    // Boolean Fast scheme
    AddParameter(ParameterType_Empty, "fast", "Use Fast Scheme");
    std::ostringstream desc;
    desc<<"If used, this option allows to speed-up computation by iteratively"
        <<" subsampling previous level of pyramid instead of processing the full input";
    SetParameterDescription("fast", desc.str());
    MandatoryOff("fast");
  }

  void DoUpdateParameters()
  {
    // Nothing to do here for the parameters : all are independent

    // Reinitialize the internal process used
  }

  void DoExecute()
  {
    // Initializing the process
    m_SmoothingFilter =  SmoothingVectorImageFilterType::New();
    m_ShrinkFilter    = ShrinkFilterType::New();

    // Extract Parameters
    unsigned int nbLevels     = GetParameterInt("level");
    unsigned int shrinkFactor = GetParameterInt("sfactor");
    double varianceFactor     = GetParameterFloat("vfactor");

    bool fastScheme = IsParameterEnabled("fast");

    // Get the input image
    FloatVectorImageType::Pointer inImage = GetParameterImage("in");

    // Get the Initial Output Image FileName
    std::string path, fname, ext;
    std::string ofname = GetParameterString("out");

    // Get the extension and the prefix of the filename
    path  = itksys::SystemTools::GetFilenamePath(ofname);
    fname = itksys::SystemTools::GetFilenameWithoutExtension(ofname);
    ext   = itksys::SystemTools::GetFilenameExtension(ofname);

    unsigned int currentLevel = 1;
    unsigned int currentFactor = shrinkFactor;

    while(currentLevel <= nbLevels)
      {
      otbAppLogDEBUG( << "Processing level " << currentLevel
                      << " with shrink factor "<<currentFactor);

      m_SmoothingFilter->SetInput(inImage);

      // According to
      // http://www.ipol.im/pub/algo/gjmr_line_segment_detector/
      // This is a good balance between blur and aliasing
      double variance = varianceFactor * static_cast<double>(currentFactor);
      m_SmoothingFilter->GetFilter()->SetVariance(variance);

      m_ShrinkFilter->SetInput(m_SmoothingFilter->GetOutput());
      m_ShrinkFilter->SetShrinkFactors(currentFactor);

      if(!fastScheme)
        {
        currentFactor *= shrinkFactor;
        }
      else
        {
        std::cout <<"fast scheme enabled : not implemented for the moment " << std::endl;
        }

      // Create an output parameter to write the current output image
      OutputImageParameter::Pointer paramOut = OutputImageParameter::New();
        
      // build the current image filename
      std::ostringstream oss;
      oss <<path<<"/"<<fname<<"_"<<currentLevel<<ext;

      // writer label
      std::ostringstream osswriter;
      osswriter<< "writer (level "<< currentLevel<<")";

      // Set the filename of the current output image
      paramOut->SetFileName(oss.str());
      paramOut->SetValue(m_ShrinkFilter->GetOutput());
      // Add the current level to be written
      paramOut->InitializeWriters();
      AddProcess(paramOut->GetWriter(),osswriter.str());
      paramOut->Write();

      ++currentLevel;
      }
  }

  SmoothingVectorImageFilterType::Pointer   m_SmoothingFilter;
  ShrinkFilterType::Pointer                 m_ShrinkFilter;
};
}
}

OTB_APPLICATION_EXPORT(otb::Wrapper::MultiResolutionPyramid)

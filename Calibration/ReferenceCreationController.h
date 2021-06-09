#pragma once

#include <memory>
#include <string>

namespace novac
{
class CCrossSectionData;
}

class ReferenceCreationController
{
public:
    ReferenceCreationController();

    /// <summary>
    /// Input: the full file path to the full calibration file.
    /// </summary>
    std::string m_calibrationFile;

    /// <summary>
    /// Input: the full file path to the high resolved cross section file to use.
    /// </summary>
    std::string m_highResolutionCrossSection;

    /// <summary>
    /// Setting: should the result be high-pass filtered (default is true as this is the default for novac references)
    /// </summary>
    bool m_highPassFilter = true;

    /// <summary>
    /// Setting: set to true if the high resolution reference is measured in vacuum.
    /// </summary>
    bool m_convertToAir = false;

    /// <summary>
    /// Output: The resulting convolved cross section.
    /// </summary>
    std::unique_ptr<novac::CCrossSectionData> m_resultingCrossSection;

    /// <summary>
    /// Performs the convolution of the refernce. This will update m_resultingCrossSection
    /// </summary>
    void ConvolveReference();
};


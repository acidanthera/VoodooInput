//
//  VoodooInputIDs.hpp
//
//  Created by Avery Black on 6/15/24.
//  Copyright © 2024 Kishor Prins. All rights reserved.
//

#ifndef VOODOO_INPUT_IDS_HPP
#define VOODOO_INPUT_IDS_HPP

// macOS 10.10-14
constexpr int kVoodooInputProductMacbook8_1 = 0x272;
// macOS 12+
constexpr int kVoodooInputProductMacbookAir10_1 = 0x281;
constexpr int kVoodooInputVendorApple = 0x5ac;

constexpr int kVoodooInputVersionMonterey = 21;

int VoodooInputGetProductId();

#endif // VOODOO_INPUT_IDS_HPP

//
//  StellitePlayer.mm
//  StelliteTest
//
//  Created by Naver on 2018. 6. 11..
//  Copyright © 2018년 Naver. All rights reserved.
//

#import "StellitePlayer.h"
#include <MyTestStelliteSource.h>

@interface StellitePlayer() {
    MyTestStelliteSource    *stelliteSource;
}
@end

@implementation StellitePlayer

- (instancetype)init
{
    self = [super init];
    if (self) {
        stelliteSource = new MyTestStelliteSource();
    }
    return self;
}

- (void)requestSample {
    stelliteSource->request();
}

@end

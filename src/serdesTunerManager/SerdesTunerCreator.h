/*
 * SerdesTunerCreator.h
 *
 *  Created on: Sep 10, 2020
 *      Author: aforward
 */

#ifndef SRC_SERDESTUNERMANAGER_SERDESTUNERCREATOR_H_
#define SRC_SERDESTUNERMANAGER_SERDESTUNERCREATOR_H_
#include "SerdesTuner.h"

#include <SingletonService.h>

namespace tuning
{

class SerdesTunerCreator
{
public:
    SerdesTunerCreator() {}

    ~SerdesTunerCreator() {}

    void CreateSerdesTuner(SerdesTuner*& pNewSerdesTuner);

protected:


};

typedef SingletonService<SerdesTunerCreator> SerdesTunerCreatorSingleton;

}



#endif /* SRC_SERDESTUNERMANAGER_SERDESTUNERCREATOR_H_ */

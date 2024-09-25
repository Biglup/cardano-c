/**
 * \file certificate.h
 *
 * \author angel.castillo
 * \date   Jul 23, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CERTIFICATE_H
#define CERTIFICATE_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/cert_type.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief This certificate registers the Hot and Cold credentials of a committee member.
 */
typedef struct cardano_auth_committee_hot_cert_t cardano_auth_committee_hot_cert_t;

/**
 * \brief This certificate is used to delegate from a Genesis key to a set of keys. This was primarily used in the early
 * phases of the Cardano network's existence during the transition from the Byron to the Shelley era.
 */
typedef struct cardano_genesis_key_delegation_cert_t cardano_genesis_key_delegation_cert_t;

/**
 * \brief Certificate used to facilitate an instantaneous transfer of rewards within the system.
 */
typedef struct cardano_mir_cert_t cardano_mir_cert_t;

/**
 * \brief This certificate is used to register a new stake pool. It includes various details
 * about the pool such as the pledge, costs, margin, reward account, and the pool's owners and relays.
 */
typedef struct cardano_pool_registration_cert_t cardano_pool_registration_cert_t;

/**
 * \brief This certificate is used to retire a stake pool. It includes an epoch number indicating when the pool will be retired.
 */
typedef struct cardano_pool_retirement_cert_t cardano_pool_retirement_cert_t;

/**
 * \brief In Voltaire, existing stake credentials will be able to delegate their stake to DReps for voting
 * purposes, in addition to the current delegation to stake pools for block production.
 * DRep delegation will mimic the existing stake delegation mechanisms (via on-chain certificates).
 *
 * This certificate register a stake key as a DRep.
 */
typedef struct cardano_register_drep_cert_t cardano_register_drep_cert_t;

/**
 * \brief This certificate is used when an individual wants to register as a stakeholder.
 * It allows the holder to participate in the staking process by delegating their
 * stake or creating a stake pool.
 */
typedef struct cardano_registration_cert_t cardano_registration_cert_t;

/**
 * \brief This certificate is used then a committee member wants to resign early (will be marked on-chain as an expired member).
 */
typedef struct cardano_resign_committee_cold_cert_t cardano_resign_committee_cold_cert_t;

/**
 * \brief This certificate is used when a stakeholder wants to delegate their stake to a
 * specific stake pool. It includes the stake pool id to which the stake is delegated.
 */
typedef struct cardano_stake_delegation_cert_t cardano_stake_delegation_cert_t;

/**
 * \brief This certificate is used when a stakeholder no longer wants to participate in
 * staking. It revokes the stake registration and the associated stake is no
 * longer counted when calculating stake pool rewards.
 */
typedef struct cardano_stake_deregistration_cert_t cardano_stake_deregistration_cert_t;

/**
 * \brief This certificate is used when an individual wants to register as a stakeholder.
 * It allows the holder to participate in the stake process by delegating their
 * stake or creating a stake pool.
 */
typedef struct cardano_stake_registration_cert_t cardano_stake_registration_cert_t;

/**
 * \brief This certificate Register the stake key and delegate with a single certificate to a stake pool.
 */
typedef struct cardano_stake_registration_delegation_cert_t cardano_stake_registration_delegation_cert_t;

/**
 * \brief This certificate is used when an individual wants to delegate their voting
 * rights to any other DRep and simultaneously wants to delegate their stake to a
 * specific stake pool.
 */
typedef struct cardano_stake_vote_delegation_cert_t cardano_stake_vote_delegation_cert_t;

/**
 * \brief This certificate is used when an individual wants to register its stake key,
 * delegate their voting rights to any other DRep and simultaneously wants to delegate
 * their stake to a specific stake pool.
 */
typedef struct cardano_stake_vote_registration_delegation_cert_t cardano_stake_vote_registration_delegation_cert_t;

/**
 * \brief This certificate unregister an individual as a DRep.
 *
 * Note that a DRep is retired immediately upon the chain accepting a retirement certificate, and
 * the deposit is returned as part of the transaction that submits the retirement certificate
 * (the same way that stake credential registration deposits are returned).
 */
typedef struct cardano_unregister_drep_cert_t cardano_unregister_drep_cert_t;

/**
 * \brief This certificate is used when a stakeholder no longer wants to participate in
 * staking. It revokes the stake Unregistration and the associated stake is no
 * longer counted when calculating stake pool rewards.
 */
typedef struct cardano_unregistration_cert_t cardano_unregistration_cert_t;

/**
 * \brief Updates the DRep anchored metadata.
 */
typedef struct cardano_update_drep_cert_t cardano_update_drep_cert_t;

/**
 * \brief This certificate is used when an individual wants to delegate their voting rights to any other DRep.
 */
typedef struct cardano_vote_delegation_cert_t cardano_vote_delegation_cert_t;

/**
 * \brief This certificate Register the stake key and delegate with a single certificate to a DRep.
 */
typedef struct cardano_vote_registration_delegation_cert_t cardano_vote_registration_delegation_cert_t;

/**
 * \brief Creates a new Cardano certificate based on an authorization committee hot certificate.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] auth_committee_hot_cert A pointer to an initialized \ref cardano_auth_committee_hot_cert_t object that defines
 *                                    the committee's hot certificate details.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_cert = ...;  // Assume auth_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_auth_committee_hot(auth_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for authorization purposes in committee operations
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create authorization certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_auth_committee_hot_cert_unref(&auth_cert); // Cleanup the auth_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_auth_committee_hot(
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert,
  cardano_certificate_t**            certificate);

/**
 * \brief Creates a new Cardano certificate for genesis key delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object for the purpose of genesis key delegation,
 * using the provided \ref cardano_genesis_key_delegation_cert_t object which contains the delegation details.
 *
 * \param[in] genesis_key_delegation A pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object that
 *                                   specifies the genesis key delegation parameters.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* genesis_cert = ...;  // Assume genesis_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_genesis_key_delegation(genesis_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for genesis key delegation operations
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create genesis key delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_genesis_key_delegation_cert_unref(&genesis_cert); // Cleanup the genesis_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_genesis_key_delegation(
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation,
  cardano_certificate_t**                certificate);

/**
 * \brief Creates a new Cardano certificate for Move Instantaneous Rewards (MIR).
 *
 * This function allocates and initializes a \ref cardano_certificate_t object for the purpose of managing
 * Move Instantaneous Rewards, using the provided \ref cardano_mir_cert_t object which contains the MIR details.
 *
 * \param[in] mir A pointer to an initialized \ref cardano_mir_cert_t object that specifies the MIR parameters.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = ...;  // Assume mir_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_mir(mir_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for MIR operations
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create MIR certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_mir_cert_unref(&mir_cert); // Cleanup the mir_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_mir(
  cardano_mir_cert_t*     mir,
  cardano_certificate_t** certificate);

/**
 * \brief Creates a new Cardano certificate for pool registration.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object specifically for registering
 * a staking pool, using the provided \ref cardano_pool_registration_cert_t object which contains all the necessary
 * parameters for pool registration.
 *
 * \param[in] pool_registration A pointer to an initialized \ref cardano_pool_registration_cert_t object that defines
 *                              the staking pool's registration details.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_registration_cert_t* registration_cert = ...;  // Assume registration_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_pool_registration(registration_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for pool registration purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create pool registration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_pool_registration_cert_unref(&registration_cert); // Cleanup the registration_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_pool_registration(
  cardano_pool_registration_cert_t* pool_registration,
  cardano_certificate_t**           certificate);

/**
 * \brief Creates a new Cardano certificate for pool retirement.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object specifically for the retirement
 * of a staking pool, using the provided \ref cardano_pool_retirement_cert_t object which contains all the necessary
 * parameters for pool retirement.
 *
 * \param[in] pool_retirement A pointer to an initialized \ref cardano_pool_retirement_cert_t object that defines
 *                            the staking pool's retirement details.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_retirement_cert_t* retirement_cert = ...;  // Assume retirement_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_pool_retirement(retirement_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for pool retirement purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create pool retirement certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_pool_retirement_cert_unref(&retirement_cert); // Cleanup the retirement_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_pool_retirement(
  cardano_pool_retirement_cert_t* pool_retirement,
  cardano_certificate_t**         certificate);

/**
 * \brief Creates a new Cardano certificate for registering a decentralized representation (drep).
 *
 * This function allocates and initializes a \ref cardano_certificate_t object for registering a drep,
 * using the provided \ref cardano_register_drep_cert_t object which contains all necessary details
 * for the registration.
 *
 * \param[in] register_drep A pointer to an initialized \ref cardano_register_drep_cert_t object that defines
 *                          the details of the drep to be registered.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* drep_cert = ...;  // Assume drep_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_register_drep(drep_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for registering the drep
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create register drep certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_register_drep_cert_unref(&drep_cert); // Cleanup the drep_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_register_drep(
  cardano_register_drep_cert_t* register_drep,
  cardano_certificate_t**       certificate);

/**
 * \brief Creates a new Cardano certificate for registration.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_registration_cert_t object, which contains the registration details.
 *
 * \param[in] registration A pointer to an initialized \ref cardano_registration_cert_t object that specifies
 *                         the registration details.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_registration_cert_t* registration_cert = ...;  // Assume registration_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_registration(registration_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for the intended registration purpose
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create registration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_registration_cert_unref(&registration_cert); // Cleanup the registration_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_registration(
  cardano_registration_cert_t* registration,
  cardano_certificate_t**      certificate);

/**
 * \brief Creates a new Cardano certificate for resigning a committee's cold key.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_resign_committee_cold_cert_t object, which contains the details for resigning a committee's cold key.
 *
 * \param[in] resign_committee_cold A pointer to an initialized \ref cardano_resign_committee_cold_cert_t object that specifies
 *                                  the details for the resignation of the committee's cold key.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_resign_committee_cold_cert_t* resign_cert = ...;  // Assume resign_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_resign_committee_cold(resign_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for committee cold key resignation
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create resignation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_resign_committee_cold_cert_unref(&resign_cert); // Cleanup the resign_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_resign_committee_cold(
  cardano_resign_committee_cold_cert_t* resign_committee_cold,
  cardano_certificate_t**               certificate);

/**
 * \brief Creates a new Cardano certificate for stake delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_delegation_cert_t object, which contains the details necessary for delegating stake in the Cardano network.
 *
 * \param[in] stake_delegation A pointer to an initialized \ref cardano_stake_delegation_cert_t object that specifies
 *                             the delegation details, including the delegator's stake key and the target stake pool.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_delegation_cert_t* delegation_cert = ...;  // Assume delegation_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_delegation(delegation_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for stake delegation purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_delegation_cert_unref(&delegation_cert); // Cleanup the delegation_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_delegation(
  cardano_stake_delegation_cert_t* stake_delegation,
  cardano_certificate_t**          certificate);

/**
 * \brief Creates a new Cardano certificate for stake deregistration.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_deregistration_cert_t object.
 *
 * \param[in] stake_deregistration A pointer to an initialized \ref cardano_stake_deregistration_cert_t object that specifies
 *                                 the stake key to be deregistered.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_deregistration_cert_t* deregistration_cert = ...;  // Assume deregistration_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_deregistration(deregistration_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for stake deregistration purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake deregistration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_deregistration_cert_unref(&deregistration_cert); // Cleanup the deregistration_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_deregistration(
  cardano_stake_deregistration_cert_t* stake_deregistration,
  cardano_certificate_t**              certificate);

/**
 * \brief Creates a new Cardano certificate for stake registration.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_registration_cert_t object.
 *
 * \param[in] stake_registration A pointer to an initialized \ref cardano_stake_registration_cert_t object that specifies
 *                               the stake key to be registered.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_cert_t* registration_cert = ...;  // Assume registration_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_registration(registration_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for stake registration purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake registration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_registration_cert_unref(&registration_cert); // Cleanup the registration_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_registration(
  cardano_stake_registration_cert_t* stake_registration,
  cardano_certificate_t**            certificate);

/**
 * \brief Creates a new Cardano certificate for stake registration with delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \param[in] stake_registration_delegation A pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object
 *                                          that specifies the stake key to be registered and the pool to which it will be delegated.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* reg_del_cert = ...;  // Assume reg_del_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_registration_delegation(reg_del_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for stake registration and delegation purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake registration and delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_registration_delegation_cert_unref(&reg_del_cert); // Cleanup the reg_del_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_registration_delegation(
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation,
  cardano_certificate_t**                       certificate);

/**
 * \brief Creates a new Cardano certificate for stake vote delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_vote_delegation_cert_t object.
 *
 * \param[in] stake_vote_delegation A pointer to an initialized \ref cardano_stake_vote_delegation_cert_t object
 *                                  that specifies the stake key and the party to whom the voting rights are delegated.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_vote_delegation_cert_t* vote_del_cert = ...;  // Assume vote_del_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_vote_delegation(vote_del_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for delegating voting rights
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake vote delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_vote_delegation_cert_unref(&vote_del_cert); // Cleanup the vote_del_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_vote_delegation(
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation,
  cardano_certificate_t**               certificate);

/**
 * \brief Creates a new Cardano certificate for stake vote registration delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_stake_vote_registration_delegation_cert_t object.
 *
 * \param[in] stake_vote_registration_delegation A pointer to an initialized \ref cardano_stake_vote_registration_delegation_cert_t object
 *                                                that specifies the stake key and the details necessary for registering and delegating
 *                                                the voting rights.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_vote_registration_delegation_cert_t* reg_del_cert = ...;  // Assume reg_del_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_stake_vote_registration_delegation(reg_del_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for registering and delegating voting rights
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake vote registration delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_stake_vote_registration_delegation_cert_unref(&reg_del_cert); // Cleanup the reg_del_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_stake_vote_registration_delegation(
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation,
  cardano_certificate_t**                            certificate);

/**
 * \brief Creates a new Cardano certificate for unregistering a DRep.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_unregister_drep_cert_t object.
 *
 * \param[in] unregister_drep A pointer to an initialized \ref cardano_unregister_drep_cert_t object
 *                             that specifies the details necessary for unregistering the DRep.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregister_drep_cert_t* unregister_drep_cert = ...;  // Assume unregister_drep_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_unregister_drep(unregister_drep_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for unregistering the DRep
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create unregister DRep certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_unregister_drep_cert_unref(&unregister_drep_cert); // Cleanup the unregister_drep_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_unregister_drep(
  cardano_unregister_drep_cert_t* unregister_drep,
  cardano_certificate_t**         certificate);

/**
 * \brief Creates a new Cardano certificate for unregistration.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_unregistration_cert_t object.
 *
 * \param[in] unregistration A pointer to an initialized \ref cardano_unregistration_cert_t object
 *                           that specifies the details necessary for the unregistration process.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregistration_cert_t* unregistration_cert = ...;  // Assume unregistration_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_unregistration(unregistration_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for unregistration purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create unregistration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_unregistration_cert_unref(&unregistration_cert); // Cleanup the unregistration_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_unregistration(
  cardano_unregistration_cert_t* unregistration,
  cardano_certificate_t**        certificate);

/**
 * \brief Creates a new Cardano certificate for updating a delegation representation (Drep).
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_update_drep_cert_t object.
 *
 * \param[in] update_drep A pointer to an initialized \ref cardano_update_drep_cert_t object
 *                        that specifies the details necessary for the update process.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_drep_cert_t* update_drep_cert = ...;  // Assume update_drep_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_update_drep(update_drep_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for updating delegation representation
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create update Drep certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_update_drep_cert_unref(&update_drep_cert); // Cleanup the update_drep_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_update_drep(
  cardano_update_drep_cert_t* update_drep,
  cardano_certificate_t**     certificate);

/**
 * \brief Creates a new Cardano certificate for vote delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_vote_delegation_cert_t object.
 *
 * \param[in] vote_delegation A pointer to an initialized \ref cardano_vote_delegation_cert_t object
 *                            that specifies the details necessary for the delegation of voting rights.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_delegation_cert_t* vote_delegation_cert = ...;  // Assume vote_delegation_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_vote_delegation(vote_delegation_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for voting delegation purposes
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create vote delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_vote_delegation_cert_unref(&vote_delegation_cert); // Cleanup the vote_delegation_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_vote_delegation(
  cardano_vote_delegation_cert_t* vote_delegation,
  cardano_certificate_t**         certificate);

/**
 * \brief Creates a new Cardano certificate for vote registration and delegation.
 *
 * This function allocates and initializes a \ref cardano_certificate_t object based on the provided
 * \ref cardano_vote_registration_delegation_cert_t object.
 *
 * \param[in] vote_registration_delegation A pointer to an initialized \ref cardano_vote_registration_delegation_cert_t object
 *                                         that specifies the details necessary for the registration and delegation of voting rights.
 * \param[out] certificate On successful execution, this will point to a newly created \ref cardano_certificate_t object.
 *                         The caller is responsible for releasing this resource using \ref cardano_certificate_unref when it
 *                         is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_registration_delegation_cert_t* vote_reg_del_cert = ...;  // Assume vote_reg_del_cert is already initialized
 * cardano_certificate_t* certificate = NULL;
 * cardano_error_t result = cardano_certificate_new_vote_registration_delegation(vote_reg_del_cert, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate can now be used for both registration and delegation of voting rights
 *   // Remember to free the certificate when done
 *   cardano_certificate_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create vote registration and delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_vote_registration_delegation_cert_unref(&vote_reg_del_cert); // Cleanup the vote_reg_del_cert
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_new_vote_registration_delegation(
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation,
  cardano_certificate_t**                      certificate);

/**
 * \brief Creates a \ref cardano_certificate_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_certificate_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a auth_committee_hot.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] auth_committee_hot A pointer to a pointer of \ref cardano_certificate_t that will be set to the address
 *                        of the newly created auth_committee_hot object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_certificate_t object by calling
 *       \ref cardano_certificate_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_certificate_t* auth_committee_hot = NULL;
 *
 * cardano_error_t result = cardano_certificate_from_cbor(reader, &auth_committee_hot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auth_committee_hot
 *
 *   // Once done, ensure to clean up and release the auth_committee_hot
 *   cardano_certificate_unref(&auth_committee_hot);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode auth_committee_hot: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_from_cbor(cardano_cbor_reader_t* reader, cardano_certificate_t** auth_committee_hot);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_certificate_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] auth_committee_hot A constant pointer to the \ref cardano_certificate_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p auth_committee_hot or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* auth_committee_hot = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_certificate_to_cbor(auth_committee_hot, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_certificate_unref(&auth_committee_hot);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_certificate_to_cbor(
  const cardano_certificate_t* auth_committee_hot,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the type of a Cardano certificate.
 *
 * This function determines the type of a given \ref cardano_certificate_t object, which indicates the specific kind
 * of operations or permissions the certificate represents within the Cardano blockchain ecosystem.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_certificate_t object whose type is to be determined.
 * \param[out] type A pointer to a \ref cardano_cert_type_t variable where the type of the certificate will be stored upon successful execution.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the type was successfully retrieved,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if either input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_cert_type_t type;
 *
 * cardano_error_t result = cardano_cert_get_type(certificate, &type);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Certificate type: %d\n", type);
 * }
 * else
 * {
 *   printf("Failed to retrieve certificate type.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_cert_get_type(const cardano_certificate_t* certificate, cardano_cert_type_t* type);

/**
 * \brief Converts a certificate to an authorization committee hot certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] auth_committee_hot_cert On successful conversion, this will point to the \ref cardano_auth_committee_hot_cert_t object derived from the certificate.
 *                                     If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_auth_committee_hot(certificate, &auth_committee_hot_cert);
 * if (result == CARDANO_SUCCESS && auth_committee_hot_cert != NULL)
 * {
 *   // Successfully converted certificate to auth committee hot certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, auth_committee_hot_cert will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_auth_committee_hot(
  cardano_certificate_t*              certificate,
  cardano_auth_committee_hot_cert_t** auth_committee_hot_cert);

/**
 * \brief Converts a certificate to a genesis key delegation certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_genesis_key_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] genesis_key_delegation On successful conversion, this will point to the \ref cardano_genesis_key_delegation_cert_t object derived from the certificate.
 *                                    If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_genesis_key_delegation_cert_t* genesis_key_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_genesis_key_delegation(certificate, &genesis_key_delegation);
 * if (result == CARDANO_SUCCESS && genesis_key_delegation != NULL)
 * {
 *   // Successfully converted certificate to genesis key delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, genesis_key_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_genesis_key_delegation(
  cardano_certificate_t*                  certificate,
  cardano_genesis_key_delegation_cert_t** genesis_key_delegation);

/**
 * \brief Converts a certificate to an MIR certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_mir_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] mir On successful conversion, this will point to the \ref cardano_mir_cert_t object derived from the certificate.
 *                 If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_mir_cert_t* mir = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_mir(certificate, &mir);
 * if (result == CARDANO_SUCCESS && mir != NULL)
 * {
 *   // Successfully converted certificate to MIR certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, mir will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_mir(
  cardano_certificate_t* certificate,
  cardano_mir_cert_t**   mir);

/**
 * \brief Converts a certificate to a pool registration certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_pool_registration_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] pool_registration On successful conversion, this will point to the \ref cardano_pool_registration_cert_t object derived from the certificate.
 *                               If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_pool_registration_cert_t* pool_registration = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_pool_registration(certificate, &pool_registration);
 * if (result == CARDANO_SUCCESS && pool_registration != NULL)
 * {
 *   // Successfully converted certificate to pool registration certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, pool_registration will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_pool_registration(
  cardano_certificate_t*             certificate,
  cardano_pool_registration_cert_t** pool_registration);

/**
 * \brief Converts a certificate to a pool retirement certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_pool_retirement_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] pool_retirement On successful conversion, this will point to the \ref cardano_pool_retirement_cert_t object derived from the certificate.
 *                             If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_pool_retirement_cert_t* pool_retirement = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_pool_retirement(certificate, &pool_retirement);
 * if (result == CARDANO_SUCCESS && pool_retirement != NULL)
 * {
 *   // Successfully converted certificate to pool retirement certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, pool_retirement will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_pool_retirement(
  cardano_certificate_t*           certificate,
  cardano_pool_retirement_cert_t** pool_retirement);

/**
 * \brief Converts a certificate to a register drep certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_register_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] register_drep On successful conversion, this will point to the \ref cardano_register_drep_cert_t object derived from the certificate.
 *                           If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_register_drep_cert_t* register_drep = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_register_drep(certificate, &register_drep);
 * if (result == CARDANO_SUCCESS && register_drep != NULL)
 * {
 *   // Successfully converted certificate to register drep certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, register_drep will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_register_drep(
  cardano_certificate_t*         certificate,
  cardano_register_drep_cert_t** register_drep);

/**
 * \brief Converts a certificate to a registration certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_registration_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] registration On successful conversion, this will point to the \ref cardano_registration_cert_t object derived from the certificate.
 *                          If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_registration_cert_t* registration = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_registration(certificate, &registration);
 * if (result == CARDANO_SUCCESS && registration != NULL)
 * {
 *   // Successfully converted certificate to registration certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, registration will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_registration(
  cardano_certificate_t*        certificate,
  cardano_registration_cert_t** registration);

/**
 * \brief Converts a certificate to a resign committee cold certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_resign_committee_cold_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] resign_committee_cold On successful conversion, this will point to the \ref cardano_resign_committee_cold_cert_t object derived from the certificate.
 *                                   If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_resign_committee_cold_cert_t* resign_committee_cold = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_resign_committee_cold(certificate, &resign_committee_cold);
 * if (result == CARDANO_SUCCESS && resign_committee_cold != NULL)
 * {
 *   // Successfully converted certificate to resign committee cold certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, resign_committee_cold will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_resign_committee_cold(
  cardano_certificate_t*                 certificate,
  cardano_resign_committee_cold_cert_t** resign_committee_cold);

/**
 * \brief Converts a certificate to a stake delegation certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_delegation On successful conversion, this will point to the \ref cardano_stake_delegation_cert_t object derived from the certificate.
 *                              If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_delegation_cert_t* stake_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_delegation(certificate, &stake_delegation);
 * if (result == CARDANO_SUCCESS && stake_delegation != NULL)
 * {
 *   // Successfully converted certificate to stake delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_delegation(
  cardano_certificate_t*            certificate,
  cardano_stake_delegation_cert_t** stake_delegation);

/**
 * \brief Converts a certificate to a stake deregistration certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_deregistration_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_deregistration On successful conversion, this will point to the \ref cardano_stake_deregistration_cert_t object derived from the certificate.
 *                                  If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_deregistration_cert_t* stake_deregistration = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_deregistration(certificate, &stake_deregistration);
 * if (result == CARDANO_SUCCESS && stake_deregistration != NULL)
 * {
 *   // Successfully converted certificate to stake deregistration certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_deregistration will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_deregistration(
  cardano_certificate_t*                certificate,
  cardano_stake_deregistration_cert_t** stake_deregistration);

/**
 * \brief Converts a certificate to a stake registration certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_registration_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_registration On successful conversion, this will point to the \ref cardano_stake_registration_cert_t object derived from the certificate.
 *                                If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_registration_cert_t* stake_registration = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_registration(certificate, &stake_registration);
 * if (result == CARDANO_SUCCESS && stake_registration != NULL)
 * {
 *   // Successfully converted certificate to stake registration certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_registration will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_registration(
  cardano_certificate_t*              certificate,
  cardano_stake_registration_cert_t** stake_registration);

/**
 * \brief Converts a certificate to a stake registration delegation certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_registration_delegation On successful conversion, this will point to the \ref cardano_stake_registration_delegation_cert_t object derived from the certificate.
 *                                           If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_registration_delegation_cert_t* stake_registration_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_registration_delegation(certificate, &stake_registration_delegation);
 * if (result == CARDANO_SUCCESS && stake_registration_delegation != NULL)
 * {
 *   // Successfully converted certificate to stake registration delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_registration_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_registration_delegation(
  cardano_certificate_t*                         certificate,
  cardano_stake_registration_delegation_cert_t** stake_registration_delegation);

/**
 * \brief Converts a certificate to a stake vote delegation certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_vote_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_vote_delegation On successful conversion, this will point to the \ref cardano_stake_vote_delegation_cert_t object derived from the certificate.
 *                                   If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_vote_delegation_cert_t* stake_vote_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_vote_delegation(certificate, &stake_vote_delegation);
 * if (result == CARDANO_SUCCESS && stake_vote_delegation != NULL)
 * {
 *   // Successfully converted certificate to stake vote delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_vote_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_vote_delegation(
  cardano_certificate_t*                 certificate,
  cardano_stake_vote_delegation_cert_t** stake_vote_delegation);

/**
 * \brief Converts a certificate to a stake vote registration delegation certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_stake_vote_registration_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] stake_vote_registration_delegation On successful conversion, this will point to the \ref cardano_stake_vote_registration_delegation_cert_t object derived from the certificate.
 *                                                 If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_stake_vote_registration_delegation(certificate, &stake_vote_registration_delegation);
 * if (result == CARDANO_SUCCESS && stake_vote_registration_delegation != NULL)
 * {
 *   // Successfully converted certificate to stake vote registration delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, stake_vote_registration_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_stake_vote_registration_delegation(
  cardano_certificate_t*                              certificate,
  cardano_stake_vote_registration_delegation_cert_t** stake_vote_registration_delegation);

/**
 * \brief Converts a certificate to an unregister DREP certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_unregister_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] unregister_drep On successful conversion, this will point to the \ref cardano_unregister_drep_cert_t object derived from the certificate.
 *                             If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_unregister_drep_cert_t* unregister_drep = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_unregister_drep(certificate, &unregister_drep);
 * if (result == CARDANO_SUCCESS && unregister_drep != NULL)
 * {
 *   // Successfully converted certificate to unregister DREP certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, unregister_drep will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_unregister_drep(
  cardano_certificate_t*           certificate,
  cardano_unregister_drep_cert_t** unregister_drep);

/**
 * \brief Converts a certificate to an unregistration certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_unregistration_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] unregistration On successful conversion, this will point to the \ref cardano_unregistration_cert_t object derived from the certificate.
 *                            If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_unregistration_cert_t* unregistration = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_unregistration(certificate, &unregistration);
 * if (result == CARDANO_SUCCESS && unregistration != NULL)
 * {
 *   // Successfully converted certificate to unregistration certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, unregistration will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_unregistration(
  cardano_certificate_t*          certificate,
  cardano_unregistration_cert_t** unregistration);

/**
 * \brief Converts a certificate to an update drep certificate type.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_update_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] update_drep On successful conversion, this will point to the \ref cardano_update_drep_cert_t object derived from the certificate.
 *                         If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_update_drep_cert_t* update_drep = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_update_drep(certificate, &update_drep);
 * if (result == CARDANO_SUCCESS && update_drep != NULL)
 * {
 *   // Successfully converted certificate to update drep certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, update_drep will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_update_drep(
  cardano_certificate_t*       certificate,
  cardano_update_drep_cert_t** update_drep);

/**
 * \brief Converts a certificate to a vote delegation certificate.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_vote_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] vote_delegation On successful conversion, this will point to the \ref cardano_vote_delegation_cert_t object derived from the certificate.
 *                             If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_vote_delegation_cert_t* vote_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_vote_delegation(certificate, &vote_delegation);
 * if (result == CARDANO_SUCCESS && vote_delegation != NULL)
 * {
 *   // Successfully converted certificate to vote delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, vote_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_vote_delegation(
  cardano_certificate_t*           certificate,
  cardano_vote_delegation_cert_t** vote_delegation);

/**
 * \brief Converts a certificate to a vote registration delegation certificate.
 *
 * This function attempts to convert a \ref cardano_certificate_t object into a \ref cardano_vote_registration_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object.
 * \param[out] vote_registration_delegation On successful conversion, this will point to the \ref cardano_vote_registration_delegation_cert_t object derived from the certificate.
 *                                          If the certificate is not of the correct type, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was successful,
 *         or an appropriate error code indicating the failure reason. If the certificate type does not match, the function will return
 *         \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* certificate = ...; // Assume certificate is already initialized
 * cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;
 *
 * cardano_error_t result = cardano_certificate_to_vote_registration_delegation(certificate, &vote_registration_delegation);
 * if (result == CARDANO_SUCCESS && vote_registration_delegation != NULL)
 * {
 *   // Successfully converted certificate to vote registration delegation certificate
 * }
 * else
 * {
 *   // Handle failure or incorrect certificate type
 * }
 * \endcode
 *
 * \note If the conversion fails or the certificate type is incorrect, vote_registration_delegation will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_to_vote_registration_delegation(
  cardano_certificate_t*                        certificate,
  cardano_vote_registration_delegation_cert_t** vote_registration_delegation);

/**
 * \brief Decrements the reference count of a cardano_certificate_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_certificate_t object
 * by decreasing its reference count. When the reference count reaches zero, the auth_committee_hot is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] auth_committee_hot A pointer to the pointer of the auth_committee_hot object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_t* auth_committee_hot = cardano_certificate_new(major, minor);
 *
 * // Perform operations with the auth_committee_hot...
 *
 * cardano_certificate_unref(&auth_committee_hot);
 * // At this point, auth_committee_hot is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_certificate_unref, the pointer to the \ref cardano_certificate_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_certificate_unref(cardano_certificate_t** auth_committee_hot);

/**
 * \brief Increases the reference count of the cardano_certificate_t object.
 *
 * This function is used to manually increment the reference count of an cardano_certificate_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_certificate_unref.
 *
 * \param auth_committee_hot A pointer to the cardano_certificate_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * cardano_certificate_ref(auth_committee_hot);
 *
 * // Now auth_committee_hot can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_certificate_ref there is a corresponding
 * call to \ref cardano_certificate_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_certificate_ref(cardano_certificate_t* auth_committee_hot);

/**
 * \brief Retrieves the current reference count of the cardano_certificate_t object.
 *
 * This function returns the number of active references to an cardano_certificate_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_certificate_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param auth_committee_hot A pointer to the cardano_certificate_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_certificate_t object. If the object
 * is properly managed (i.e., every \ref cardano_certificate_ref call is matched with a
 * \ref cardano_certificate_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * size_t ref_count = cardano_certificate_refcount(auth_committee_hot);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_certificate_refcount(const cardano_certificate_t* auth_committee_hot);

/**
 * \brief Sets the last error message for a given cardano_certificate_t object.
 *
 * Records an error message in the auth_committee_hot's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_certificate_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the auth_committee_hot's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_certificate_set_last_error(
  cardano_certificate_t* auth_committee_hot,
  const char*            message);

/**
 * \brief Retrieves the last error message recorded for a specific auth_committee_hot.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_certificate_set_last_error for the given
 * auth_committee_hot. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_certificate_t instance whose last error
 *                   message is to be retrieved. If the auth_committee_hot is NULL, the function
 *                   returns a generic error message indicating the null auth_committee_hot.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified auth_committee_hot. If the auth_committee_hot is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_certificate_set_last_error for the same auth_committee_hot, or until
 *       the auth_committee_hot is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_certificate_get_last_error(
  const cardano_certificate_t* auth_committee_hot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CERTIFICATE_H
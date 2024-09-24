Transaction
===========

A transaction is an event created, signed, and sent by a user to modify the ledger's state. It is commonly used to transfer ada or other tokens from one user to another. Additionally, it can serve various purposes, such as token creation, delegation registration to a stake pool, or interaction with smart contracts, among others. The process to modify the ledger through a transaction is as follows:

 - **Creating the transaction.** A transaction includes a set of data that specifies how you intend to modify the ledger. The fundamental components of a transaction include UTXOs, from which the funds are sourced, and destination addresses to which you want to send funds along with the desired amount of tokens. There are various tools available to assist in creating transactions, such as wallets and cardano-cli.

 - **Signing the transaction.** A user who owns the funds to be spent must provide authorization for the transaction through a signature. If the funds are held in a smart contract address, the authorization is carried out by executing the smart contract itself. Various tools are available to assist in signing transactions, including wallets and cardano-cli.

 - **Submitting the transaction.** For a transaction to be reflected in the ledger, the user must submit it. These transactions are received by stake pools, validated, and then added to the ledger within a block.

Each transaction includes an identifier known as TxId, and after completing these three steps, you can view transaction's content in the Cardano Explorer, accessible at https://explorer.cardano.org, for example.

.. toctree::
    :maxdepth: 1

    ./transaction

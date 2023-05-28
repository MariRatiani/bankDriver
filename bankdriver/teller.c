
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "branch.h"
#include "error.h"
#include "debug.h"



 #define m_lock pthread_mutex_lock
 #define m_unlock pthread_mutex_unlock


/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  BranchID branchID = AccountNum_GetBranchID(accountNum);

  m_lock(&(account->lock));
  m_lock(&(bank->branches[branchID].lock));
  Account_Adjust(bank, account, amount, 1);
  m_unlock(&(account->lock));
  m_unlock(&(bank->branches[branchID].lock));

  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  BranchID branchID = AccountNum_GetBranchID(accountNum);

  if(account == NULL){
    printf("%d\n" , -1);
  }
  m_lock(&(account->lock));
  m_lock(&(bank->branches[branchID].lock));
  
  if (Account_Balance(account) < amount) {
    m_unlock(&(account->lock));
    m_unlock(&(bank->branches[branchID].lock));
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank, account, -amount, 1);
  m_unlock(&(account->lock));
  m_unlock(&(bank->branches[branchID].lock));

  return ERROR_SUCCESS;
}


void lock(int update, Bank* bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum){
  
}


int transfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount, AccountNumber srcNumb, 
                  AccountNumber dstNumb, Account *dstAccount, Account *srcAccount){

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */

  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);

  BranchID srcBranchID = AccountNum_GetBranchID(srcAccountNum);
  BranchID dstBranchID = AccountNum_GetBranchID(dstAccountNum);
  
  if (srcNumb >= dstNumb) {
    m_lock((&dstAccount->lock));
    m_lock((&srcAccount->lock));
    if (updateBranch) {
      m_lock(&(bank->branches[dstBranchID].lock));
      m_lock(&(bank->branches[srcBranchID].lock));
    }
  } else {
     m_lock(&(srcAccount->lock));
    m_lock(&(dstAccount->lock));
    if (updateBranch) {
      m_lock(&(bank->branches[srcBranchID].lock));
      m_lock(&(bank->branches[dstBranchID].lock));
    }
  }
  
  int balance = Account_Balance(srcAccount);
  if (balance < amount) {
    m_unlock(&(dstAccount->lock));
    m_unlock(&(srcAccount->lock));
    if (updateBranch) {
      m_unlock(&(bank->branches[srcBranchID].lock));
      m_unlock(&(bank->branches[dstBranchID].lock));
    }
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank, srcAccount, -amount, updateBranch);
  Account_Adjust(bank, dstAccount, amount, updateBranch);

  m_unlock(&(dstAccount->lock));
  m_unlock(&(srcAccount->lock));
  if (updateBranch) {
    m_unlock(&(bank->branches[srcBranchID].lock));
    m_unlock(&(bank->branches[dstBranchID].lock));
  }

  return ERROR_SUCCESS;
 }

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  if (srcAccountNum == dstAccountNum)
    return ERROR_SUCCESS;

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if ((srcAccount == NULL) || (dstAccount == NULL)) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  AccountNumber srcNumb = srcAccount->accountNumber;
  AccountNumber dstNumb = dstAccount->accountNumber;

  return transfer(bank, srcAccountNum, dstAccountNum, amount, srcNumb, dstNumb,
          dstAccount, srcAccount);
}
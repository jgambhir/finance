#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lists.h"

/* Add a group with name group_name to the group_list referred to by 
* group_list_ptr. The groups are ordered by the time that the group was 
* added to the list with new groups added to the end of the list.
*
* (I.e, allocate and initialize a Group struct, and insert it
* into the group_list. Note that the head of the group list might change
* which is why the first argument is a double pointer.) 
*/
char *add_group(Group **group_list_ptr, const char *group_name) {
	
	char *message;
	int length;
	
   if (find_group(*group_list_ptr, group_name) == NULL) {
        //malloc space for new group
        Group *newgrp;
        if ((newgrp = malloc(sizeof(Group))) == NULL) {
           perror("Error allocating space for new Group");
           exit(1);
        }
        // set the fields of the new group node
        // first allocate space for the name
        int needed_space = strlen(group_name) + 1;
        if (( newgrp->name = malloc(needed_space)) == NULL) {
           perror("Error allocating space for new Group name");
           exit(1);
        }
        strncpy(newgrp->name, group_name, needed_space);
        newgrp->users = NULL;
        newgrp->xcts = NULL;
        newgrp->next = NULL;

		//Create the return message string.
		//without the +1 at the end, the delim at the end gets cut off.
		length = snprintf(NULL, 0, "Group %s added.\r\n", group_name) + 1;
		message = malloc(length + 1);
		if (message == NULL) {
			perror("Malloc error.");
			exit(1);
		}
		snprintf(message, length, "Group %s added.\r\n", group_name);
		message[strlen(message)] = '\0';
		
        // Need to insert into the end of the list not the front
        if (*group_list_ptr == NULL) {
          // set new head to this new group -- the list was previously empty
          *group_list_ptr = newgrp;
          return message;
        }  else {
        // walk along the list until we find the currently last group 
          Group * current = *group_list_ptr;
          while (current->next != NULL) {
            current = current->next;
          }
          // now current should be the last group 
          current->next = newgrp;
          return message;
        }
    } else {
		//create the "group group_name already exists" return string
		length = snprintf(NULL, 0, "Group %s already exists.\r\n", group_name) + 1;
		message = malloc(length + 1);
		if (message == NULL) {
			perror("Malloc error.");
			exit(1);
		}
		snprintf(message, length, "Group %s already exists.\r\n", group_name);
		message[strlen(message)] = '\0';
		
        return message;
    }
}

/* Print to standard output the names of all groups in group_list, one name
*  per line. Output is in the same order as group_list.
*/
char *list_groups(Group *group_list) {
    Group * current = group_list;
	int length = 0;
	//go through the group list once to add up string lengths
    while (current != NULL) {
		length = length + snprintf(NULL, 0, "%s\r\n", current->name);
        current = current->next;
    }
	
	current = group_list;
	char *group_string;
	group_string = malloc(length + 1);
	if (group_string == NULL) {
		perror("Malloc error.");
		exit(1);
	}
	char *newline = "\r\n";
	//go through the group list a second time to collect all of the group names
	while (current != NULL) {
		strncat(group_string, current->name, strlen(current->name));
		strncat(group_string, newline, strlen(newline));
		current = current->next;
	}
	
	group_string[strlen(group_string)] = '\0';
	return group_string;
}

/* Search the list of groups for a group with matching group_name
* If group_name is not found, return NULL, otherwise return a pointer to the 
* matching group list node.
*/
Group *find_group(Group *group_list, const char *group_name) {
    Group *current = group_list;
    while (current != NULL && (strcmp(current->name, group_name) != 0)) {
        current = current->next;
    }
    return current;
}

/* Add a new user with the specified user name to the specified group. 
* (allocate and initialize a User data structure and insert it into the
* appropriate group list)
*/
char *add_user(Group *group, const char *user_name) {
	char *message;
	int length;
	
    User *this_user = find_prev_user(group, user_name);
    if (this_user != NULL) {
		length = snprintf(NULL, 0, "%s is already in group %s.\r\n", user_name, group->name) + 1;
		message = malloc(length + 1);
		if (message == NULL) {
			perror("Malloc error.");
			exit(1);
		}
		snprintf(message, length, "%s is already in group %s.\r\n", user_name, group->name);
		message[strlen(message)] = '\0';
        return message;
    }
    // ok to add a user to this group by this name
    // since users are stored by balance order and the new user has 0 balance
    // he goes first
    User *newuser;
    if ((newuser = malloc(sizeof(User))) == NULL) {
        perror("Error allocating space for new User");
        exit(1);
    }
    // set the fields of the new user node
    // first allocate space for the name
    int name_len = strlen(user_name);
    if ((newuser->name = malloc(name_len + 1)) == NULL) {
        perror("Error allocating space for new User name");
        exit(1);
    }
    strncpy(newuser->name, user_name, name_len + 1);
    newuser->balance = 0.0;

    // insert this user at the front of the list
    newuser->next = group->users;
    group->users = newuser;
	
	length = snprintf(NULL, 0, "Successfully added %s to the group %s.\r\n", user_name, group->name) + 1;
	message = malloc(length + 1);
	if (message == NULL) {
		perror("Malloc error.");
		exit(1);
	}
	snprintf(message, length, "Successfully added %s to the group %s.\r\n", user_name, group->name);
	message[strlen(message)] = '\0';
    return message;
}

/* Print to standard output the names and balances of all the users in group,
* one per line, and in the order that users are stored in the list, namely 
* lowest payer first.
*/
char *list_users(Group *group) {
	int length = 0;
	
	// go over the user list once to add up lengths.
    User *current_user = group->users;
    while (current_user != NULL) {
		length = length + snprintf(NULL, 0, "%s: %.2f\r\n", current_user->name, current_user->balance) + 1;
        current_user = current_user->next;
    }
	
	char *users;
	char *curr;
	users = malloc(length + 1);
	curr = malloc(length);
	if (users == NULL || curr == NULL) {
		perror("Malloc error.");
		exit(1);
	}
	
	/* Long comment about not overwriting space, as per assignment specs:
	 *
	 * In this function, it is guaranteed that the space allocated for the return value
     * will not be overwritten. This is due to the way memory is being allocated (dyamic allocation); 
	 * using snprintf to add up string lengths, we calculate how much space will be needed to write 
	 * what we need to the string. After allocating the required amount of space, We then use
	 * that same snprintf expression (replacing NULL and 0 with the return string and the max length 
	 * that snprintf is allowed to copy to the string), to append to the string. It is guaranteed that
	 * we are allocating at least as much memory as we need, and the space allocated will not be
     * overwritten.
	 */
	
	// go over the user list again to write to the return string.
	current_user = group->users;
	int curr_length;
	while (current_user != NULL) {
		curr_length = snprintf(NULL, 0, "%s: %.2f\r\n", current_user->name, current_user->balance);
		snprintf(curr, curr_length + 1, "%s: %.2f\r\n", current_user->name, current_user->balance);
		strncat(users, curr, curr_length + 1);
		current_user = current_user->next;
	}
	users[strlen(users)] = '\0';
    return users;
}

/* Return a string containing the balance of the specified user.
*/
char *user_balance(Group *group, const char *user_name) {
	char *message;
	int length;
	
    User * prev_user = find_prev_user(group, user_name);
    if (prev_user == NULL) {
		length = snprintf(NULL, 0, "User %s is not in the group %s\r\n", user_name, group->name);
		message = malloc(length + 1);
		if (message == NULL) {
			perror("Malloc error.");
			exit(1);
		}
		snprintf(message, length, "User %s is not in the group %s\r\n", user_name, group->name);
		message[sizeof(message)] = '\0';
        return message;	
    }
	
    if (prev_user == group->users) {
        // user could be first or second since previous is first
        if (strcmp(user_name, prev_user->name) == 0) {
            // this is the special case of first user
			length = snprintf(NULL, 0, "Balance is %.02f\r\n", prev_user->balance);
			message = malloc(length + 1);
			if (message == NULL) {
				perror("Malloc error.");
				exit(1);
			}
            snprintf(message, length, "Balance is %.02f\r\n", prev_user->balance);
			message[strlen(message)] = '\0';
            return message;
        }
    }
	length = snprintf(NULL, 0, "Balance is %.02f\r\n", prev_user->next->balance);
	message = malloc(length + 1);
	if (message == NULL) {
		perror("Malloc error.");
		exit(1);
	}
    snprintf(message, length, "Balance is %.02f\r\n", prev_user->next->balance);
	message[strlen(message)] = '\0';
    return message;
}

/* Return a pointer to the user prior to the one in group with user_name. If 
* the matching user is the first in the list (i.e. there is no prior user in 
* the list), return a pointer to the matching user itself. If no matching user 
* exists, return NULL. 
*
* The reason for returning the prior user is that returning the matching user 
* itself does not allow us to change the user that occurs before the
* matching user, and some of the functions you will implement require that
* we be able to do this.
*/
User *find_prev_user(Group *group, const char *user_name) {
    User * current_user = group->users;
    // return NULL for no users in this group
    if (current_user == NULL) { 
        return NULL;
    }
    // special case where user we want is first
    if (strcmp(current_user->name, user_name) == 0) {
        return current_user;
    }
    while (current_user->next != NULL) {
        if (strcmp(current_user->next->name, user_name) == 0) {
            // we've found the user so return the previous one
            return current_user;
        }
    current_user = current_user->next;
    }
    // if we get this far without returning, current_user is last,
    // and we have already checked the last element
    return NULL;
}

/* Add the transaction represented by user_name and amount to the appropriate 
* transaction list, and update the balances of the corresponding user and group. 
* Note that updating a user's balance might require the user to be moved to a
* different position in the list to keep the list in sorted order.
*/
char *add_xct(Group *group, const char *user_name, double amount) {
	char *message;
	int length;
	
    User *this_user;
    User *prev = find_prev_user(group, user_name);
    if (prev == NULL) {
		length = snprintf(NULL, 0, "User %s is not in the group %s\r\n", user_name, group->name) + 1;
		message = malloc(length + 1);
		if (message == NULL) {
			perror("Malloc error.");
			exit(1);
		}
		snprintf(message, length, "User %s is not in the group %s\r\n", user_name, group->name);
		message[strlen(message)] = '\0';
        return message;
    }
    // but find_prev_user gets the PREVIOUS user, so correct
    if (prev == group->users) {
        // user could be first or second since previous is first
        if (strcmp(user_name, prev->name) == 0) {
            // this is the special case of first user
            this_user = prev;
        } else {
            this_user = prev->next;
        }
    } else {
        this_user = prev->next;
    }

    Xct *newxct;
    if ((newxct = malloc(sizeof(Xct))) == NULL) {
        perror("Error allocating space for new Xct");
        exit(1);
    }
    // set the fields of the new transaction node
    // first allocate space for the name
    int needed_space = strlen(user_name) + 1;
    if ((newxct->name = malloc(needed_space)) == NULL) {
         perror("Error allocating space for new xct name");
         exit(1);
    }
    strncpy(newxct->name, user_name, needed_space);
    newxct->amount = amount;

    // insert this xct  at the front of the list
    newxct->next = group->xcts;
    group->xcts = newxct;

    // first readjust the balance
    this_user->balance = this_user->balance + amount;

    // since we are only ever increasing this user's balance they can only
    // go further towards the end of the linked list
    //   So keep shifting if the following user has a smaller balance

    while (this_user->next != NULL &&
                  this_user->balance > this_user->next->balance ) {
        // he remains as this user but the user next gets shifted
        // to be behind him
        if (prev == this_user) {
            User *shift = this_user->next;
            this_user->next = shift->next;
            prev = shift;
            prev->next = this_user;
            group->users = prev;
        } else { // ordinary case in the middle
            User *shift = this_user->next;
            prev->next = shift;
			prev = shift; //line added to fix starter code bug.
            this_user->next = shift->next;
            shift->next = this_user;
        }
    }
	length = snprintf(NULL, 0, "Transaction successfully added.\r\n") + 1;
	message = malloc(length + 1);
	if (message == NULL) {
		perror("Malloc error.");
		exit(1);
	}
	snprintf(message, length, "Transaction successfully added.\r\n");
	message[strlen(message)] = '\0';
	return message;
}


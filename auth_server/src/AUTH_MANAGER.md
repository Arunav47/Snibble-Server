# AUTH_MANAGER

**This documentation is for the functions of the AuthManager classes functions if ever needed to change in future**

- Handles Login
- Handles Signup
- Handles public key storing and fetching


## Constructor
- **Loads the env data**
- **There is a singleton DatabaseManager**

## Login
- **Checks in the database if the user is present**
- **If the user exists then it verifies the password with the hashedPassword that was already stored in the database for the corresponding user**
- **If the matches then it returns true otherwise false**

## Signup

- **First hashes the password and then saves the user data in the database and returns true the query executed successfully**
- **If the query fails then it returns false**

## Is User Registered
- **Checks if the user exist in the database**
- **If the user exist it returns true**
- **If the user doesnot exists it returns false**

## Search User
- **Returns a list of users whose name is like the searchItem**

## Store Public Key
- **It stores the user's public key in the database**

## Get Public Key
- **It retrieves the public key of the user with which the current user wants to communicate**


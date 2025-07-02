HTTP Request, Sessions and API Token.
Nnamdi Okpala
Nnamdi Okpala

2 min read
·
Mar 17, 2023






API come in different forms.

It like a Television and Remote Controller. We don’t need to know the remote controller function to operate the Television.

When we hit an endpoint using a network request/protocol such as HTTP/HTTPS, we then let the API carry out the operation, and return the results to us for our queries.

1) API Tokens is a bit of a general term. Usually, an API token is a unique identifier of an application requesting access to your service. Your service would generate an API token for the application to use when requesting your service. You can then match the token they provide to the one you store in order to authenticate.

A session id can be used but its purpose is different to the API token. The session id is not a form of authentication but rather a result of authorization. Typically, a session is established once a user has been authorized to use a resource (such as your service). Therefore, a session id is created when a user is granted access to a resource. An API token is the form of authentication similar to a username/password.

2) API tokens are a replacement to sending some username/password combination over HTTP which is not secure. However, the problem still exists that someone could take and use the API token instead.

3) In a way yes. It’s a method for keeping API tokens “fresh”. Instead of passing around the same API token you request an access token when you want to use a service. The OAuth 2.0 steps are as follows:

a) Request sent to service with credentials of some kind.
b) Successful response returns a code.
c) Another request to service is made with the code.
d) Successful response returns the access token for signing each API request from then until finish.

A lot of the bigger service providers use OAuth 2.0 at the moment. It’s not a perfect solution but it’s probably the most secure, wide-spread, API security method used at the moment.

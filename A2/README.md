# Assignment 2: DNS Resolver in Python

## Contributors: Aditya Gudadhe (210397)  Aryan Bharadwaj (210200)

## Overview
This project implements a DNS resolver in Python using the `dnspython` library. It provides two resolution methods:
- **Iterative DNS Lookup**: Mimics how DNS resolution works step-by-step by querying root, TLD, and authoritative servers.
- **Recursive DNS Lookup**: Uses the system's default DNS resolver to fetch results automatically.

## Features
- **Iterative Resolution:** Resolves a domain by querying the root servers and following the DNS hierarchy.
- **Recursive Resolution:** Relies on an external resolver (e.g., Google's public DNS) to fetch results.
- **Root Server Querying:** Starts iterative resolution from well-known root DNS servers.
- **Nameserver Resolution:** Extracts NS records and resolves them to IP addresses.
- **Error Handling:** Handles timeouts, unreachable servers, and resolution failures.

## Prerequisites
Ensure you have Python installed (Python 3.6+ is recommended).
Install the `dnspython` package if not already installed:
```sh
pip install dnspython
```

## How to Run
Execute the script using the following syntax:
```sh
python3 dnsresolver.py <mode> <domain>
```
Where:
- `<mode>` is either `iterative` or `recursive`.
- `<domain>` is the domain name you want to resolve.

### Example Usage
#### Iterative Lookup
```sh
python3 dnsresolver.py iterative example.com
```
Output:
```
[Iterative DNS Lookup] Resolving example.com
[DEBUG] Querying ROOT server (198.41.0.4) - SUCCESS
Extracted NS hostname: a.iana-servers.net
Extracted NS hostname: b.iana-servers.net
[DEBUG] Querying TLD server (199.9.14.201) - SUCCESS
[DEBUG] Querying AUTH server (192.33.4.12) - SUCCESS
[SUCCESS] example.com -> 93.184.216.34
Time taken: 1.234 seconds
```

#### Recursive Lookup
```sh
python3 dnsresolver.py recursive example.com
```
Output:
```
[Recursive DNS Lookup] Resolving example.com
[SUCCESS] example.com -> 93.184.216.34
Time taken: 0.123 seconds
```

## Explanation of Code
### 1. **Root Server Definitions**
A dictionary of root DNS server IPs is predefined for iterative resolution.

### 2. **send_dns_query(server, domain)**
- Constructs and sends a DNS query using UDP.
- Returns the response if successful, otherwise `None`.

### 3. **extract_next_nameservers(response)**
- Extracts NS records from the authority section.
- Resolves NS names to IP addresses.
- Returns a list of resolved NS IPs.

### 4. **iterative_dns_lookup(domain)**
- Starts DNS resolution at root servers.
- Moves through TLD and authoritative servers until an answer is found.
- Uses `send_dns_query` and `extract_next_nameservers`.

### 5. **recursive_dns_lookup(domain)**
- Uses the system's default DNS resolver to fetch results recursively.

### 6. **Main Execution**
- Parses command-line arguments.
- Runs either iterative or recursive lookup based on user input.
- Measures and prints execution time.

## Error Handling
The script implements robust error handling to ensure stability during DNS resolution:
- **Timeouts:** If a DNS query exceeds the defined timeout, it is caught and handled gracefully.
- **Unreachable Servers:** If a nameserver is unreachable, the script reports the issue and continues.
- **NXDOMAIN (Non-existent Domain):** If the domain does not exist, the script handles the exception and informs the user.
- **Invalid Responses:** If a DNS response lacks expected sections (e.g., no answer or authority), the script detects and handles it.
- **General Exceptions:** Any unexpected errors are caught and logged for debugging.

# 8. Individual Contributors:
Aditya Gudadhe (210397) : Implemented send_dns_query() and extract_next_nameservers() functions and corresponding error handling
Aryan Bharadwaj (210200) : Implemented iterative_dns_lookup() and recursive_dns_lookup() functions and corresponding error handling

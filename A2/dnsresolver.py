import dns.message
import dns.query
import dns.rdatatype
import dns.resolver
import time
import socket

# Root DNS servers used to start the iterative resolution process
ROOT_SERVERS = {
    "198.41.0.4": "Root (a.root-servers.net)",
    "199.9.14.201": "Root (b.root-servers.net)",
    "192.33.4.12": "Root (c.root-servers.net)",
    "199.7.91.13": "Root (d.root-servers.net)",
    "192.203.230.10": "Root (e.root-servers.net)"
}

TIMEOUT = 3  # Timeout in seconds for each DNS query attempt

def send_dns_query(server, domain):
    """ 
    Sends a DNS query to the given server for an A record of the specified domain.
    Returns the response if successful, otherwise returns None.
    """
    try:
        query = dns.message.make_query(domain, dns.rdatatype.A)  # Construct the DNS query
        response = dns.query.udp(query, server, timeout=TIMEOUT)  # Send the query using UDP
        return response
    except dns.exception.Timeout:
        print(f"[ERROR] Query timed out for server {server}")
    except socket.gaierror:
        print(f"[ERROR] Network error while querying {server}")
    except Exception as err:
        print(f"[ERROR] Couldn't query server {server}: {err}")
    return None

def extract_next_nameservers(response):
    """ 
    Extracts nameserver (NS) records from the authority section of the response.
    Then, resolves those NS names to IP addresses.
    Returns a list of IPs of the next authoritative nameservers.
    """
    ns_ips = []  # List to store resolved nameserver IPs
    ns_names = []  # List to store nameserver domain names
    
    # Loop through the authority section to extract NS records
    if not response.authority:
        print("[ERROR] No authority section found in response.")
        return []
    
    for rrset in response.authority:
        if rrset.rdtype == dns.rdatatype.NS:
            for rr in rrset:
                ns_name = rr.to_text()
                ns_names.append(ns_name)  # Extract nameserver hostname
                print(f"Extracted NS hostname: {ns_name}")
    
    # Resolve the extracted NS hostnames to IP addresses
    for ns_name in ns_names:
        try:
            resolved_names = dns.resolver.resolve(ns_name.rstrip('.'), "A")  # Resolve NS name to IP
            for it in resolved_names:
                ns_ips.append(it.to_text())  # Store resolved IP
                print(f"Resolved {ns_name} to {it.to_text()}")
        except (dns.resolver.NXDOMAIN, dns.resolver.NoAnswer, dns.resolver.LifetimeTimeout) as e:
            print(f"[WARNING] Failed to resolve NS {ns_name}: {e}")
    
    return ns_ips  # Return list of resolved nameserver IPs

def iterative_dns_lookup(domain):
    """ 
    Performs an iterative DNS resolution starting from root servers.
    """
    print(f"[Iterative DNS Lookup] Resolving {domain}")

    next_ns_list = list(ROOT_SERVERS.keys())  # Start with the root server IPs
    stage = "ROOT"  # Track resolution stage (ROOT, TLD, AUTH)

    while next_ns_list:
        ns_ip = next_ns_list.pop(0)  # Pick the first available nameserver to query
        response = send_dns_query(ns_ip, domain)
        
        if response:
            print(f"[DEBUG] Querying {stage} server ({ns_ip}) - SUCCESS")
            
            if response.answer:
                print(f"[SUCCESS] {domain} -> {response.answer[0][0]}")
                return
            
            next_ns_list = extract_next_nameservers(response)
            if not next_ns_list:
                print("[ERROR] No further nameservers found.")
                return
            if stage == "ROOT":
                stage = "TLD"
            elif stage == "TLD":
                stage = "AUTH"
        else:
            print(f"[ERROR] Query failed for {stage} server {ns_ip}")
            return
    
    print("[ERROR] Resolution failed.")

def recursive_dns_lookup(domain):
    """ 
    Performs recursive DNS resolution using the system's default resolver.
    """
    print(f"[Recursive DNS Lookup] Resolving {domain}")
    try:
        # Start lookup fo the Name Server
        answer = dns.resolver.resolve(domain, "NS")
        for rdata in answer:
            print(f"[SUCCESS] {domain} -> {rdata.to_text()}")
        answer = dns.resolver.resolve(domain, "A")
        for rdata in answer:
            print(f"[SUCCESS] {domain} -> {rdata.to_text()}")
    except (dns.resolver.NXDOMAIN, dns.resolver.NoAnswer):
        print(f"[ERROR] No answer found for {domain}")
    except dns.resolver.LifetimeTimeout:
        print(f"[ERROR] Lookup timed out for {domain}")
    except Exception as err:
        print(f"[ERROR] Recursive lookup failed: {err}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3 or sys.argv[1] not in {"iterative", "recursive"}:
        print("Usage: python3 dns_server.py <iterative|recursive> <domain>")
        sys.exit(1)

    mode = sys.argv[1]  # Get mode (iterative or recursive)
    domain = sys.argv[2]  # Get domain to resolve
    start_time = time.time()  # Record start time
    
    if mode == "iterative":
        iterative_dns_lookup(domain)
    else:
        recursive_dns_lookup(domain)
    
    print(f"Time taken: {time.time() - start_time:.3f} seconds")

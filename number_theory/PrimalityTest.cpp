#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;

class PrimalityTest {
private:
    long long aN;
    
    long long aGCD (long long n1, long long n2) {
        long long temp;
        while (n2 != 0) {
            temp = n1;
            n1 = n2;
            n2 = temp % n2;
        }
        return n1;
    }
    
    long long aPOW (long long base, long long exponent, long long modulus) {
        long long result = 1;
        while (exponent > 0) {
            if (exponent % 2 == 1)
                result = (result * base) % modulus;
            
            base = (base * base) % modulus;
            
            exponent /= 2;
        }
        return result;
    }
    
public:
    void SetTestNumber(long long N) {
        aN = N;
    }
    
    long long GetTestNumber() {
        return aN;
    }
    
    // Trial Division Method
    bool SimplePrimalityTest() {
        if (aN < 2)
            return false;
        
        if (aN==2 || aN==3)
            return true;
        
        for (int i=2; i<=sqrt(aN); i++) {
            if (aN%i == 0)
                return false;
        }
        
        return true;
    }
    
    // Optimize Trial Division Method
    // p = 6k +- 1, (p > 3)
    // n = 6k + i, 
    // (6k + 0) & (6k + 2) & (6k + 4) ≡ 0 (mod 2)
    // (6k + 3) ≡ 0 (mod 3)
    // remain (6k - 1) & (6k + 1)
    bool SimplePrimalityTestOptimize() {
        if (aN==2 || aN==3)
            return true;
    
        if (aN<2 || aN%2 == 0 || aN%3 == 0)
            return false;
    
        for (int i=5; i<=sqrt(aN); i+=6) {
            if (aN%i == 0 || aN%(i+2) == 0)
                return false;
        }
        
        return true;
    }
    
    // Wilson's Theorem
    // if p is prime, (p - 1)! ≡ -1 (mod p)
    // work on (n <= 20), 21! > 2^64
    bool WilsonsPrimalityTest() {
        if (aN<2 || aN>20)
            return false;
        
        long long p=1;
        for (int i=1; i<aN; i++) 
            p *= i;
        
        if (p%aN == aN-1)
            return true;
            
        return false;
    }
    
    // probable prime
    // Fermat’s little theorem
    // a^(p-1) ≡ 1 (mod p), a != 0
    bool FermatsPrimalityTest() {
        if (aN == 2)
            return true;
        
        if (aN<2 || aN%2 == 0)
            return false;
        
        long long ap = 1;
        for (long long i=1; i<aN; i++) {
            ap = (ap << 1) % aN;
        }
        
        if (ap == 1)
            return true;
        
        return false;
    }
    
    // probable prime
    // Solovay-Strassen primality test
    bool SolovayStrassenPrimalityTest(int iterations) {
        if (aN==2 || aN==3)
            return true;
        
        if (aN<2 || aN%2 == 0 || aN%3 == 0)
            return false;
            
        for (int i=0; i<iterations; i++) {
            long long a = 2 + rand() % (aN - 3),
                    jacobi = aPOW(a, (aN-1) / 2, aN);
            
            if (jacobi == 0 || jacobi != (a%aN))
                return false;
                
            if (jacobi != 1 && jacobi != aN-1)
                return false;
        }
        
        return true;
    }

};

int main() {
    srand(time(0));
    
    PrimalityTest a = PrimalityTest();
    a.SetTestNumber(4158);
    cout << "Test " << a.GetTestNumber() << endl;
    cout << a.FermatsPrimalityTest();

    return 0;
}
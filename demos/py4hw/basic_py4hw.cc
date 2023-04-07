MODULE TestCircuit (
	const uint8& a,
	uint8& r) {
// Code generated from clock method
// variable declaration 
// process 
while(always())
{
r = r+a; // sync
}
}


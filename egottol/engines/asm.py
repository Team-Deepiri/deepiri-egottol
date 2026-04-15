from typing import List, Dict

class AssemblyEngine:
    """Translates 'Deepiri-8' assembly into binary opcodes."""
    
    OPCODES = {
        "MOV": 0x01,
        "ADD": 0x02,
        "SUB": 0x03,
        "JMP": 0x04,
        "NOP": 0x00,
        "HALT": 0xFF
    }

    def assemble(self, source_code: str) -> List[int]:
        """Assembles mnemonic source into list of byte opcodes."""
        binary = []
        for line in source_code.splitlines():
            line = line.strip().split()
            if not line: continue
            
            mnemonic = line[0].upper()
            if mnemonic in self.OPCODES:
                binary.append(self.OPCODES[mnemonic])
                # Handle operands (optional simplified logic for demo)
                for arg in line[1:]:
                    binary.append(int(arg, 0) & 0xFF)
        
        return binary

    def disassemble(self, binary: List[int]) -> str:
        # Placeholder for disassembly logic
        return "DISASSEMBLY_OUTPUT"

# Cosmolab Board Definitions

This directory contains two board definition files for Cosmolab:

## 📋 Files

### `cosmolab.json` ✅ **Use this for:**
- ✅ **Plugdata/Pure Data simulation** 
- ✅ **Hardware v2+** (with corrected button polarity)
- ✅ **Development and testing**

**Button behavior:** Normal polarity (button press = LOW, released = HIGH)

---

### `cosmolab_hw_v1.json` ⚠️ **Use this ONLY for:**
- ⚠️ **Current hardware v1** (with inverted button polarity issue)

**Button behavior:** Inverted polarity to compensate for hardware pull-up resistors

---

## 🚀 How to Use

### For Plugdata/Simulation (ALWAYS use normal)
Your Max patches will automatically work with `cosmolab` board when simulating in Plugdata.

### For Hardware v1 (Current)
When compiling firmware for the actual hardware v1:

```bash
# In your Max patch, or via command line:
oopsy your_patch.maxpat --board cosmolab_hw_v1

# Then build and flash:
cd build
make clean && make
make program-dfu
```

### For Hardware v2+ (Future)
When you get hardware v2 with corrected polarity:

```bash
# Simply use the standard board:
oopsy your_patch.maxpat --board cosmolab

cd build
make clean && make
make program-dfu
```

---

## 🔧 Technical Details

### The Hardware Issue
Hardware v1 has pull-up resistors on the CD4021 shift register button inputs:
- Button **not pressed** → pin reads HIGH (1)
- Button **pressed** → pin reads LOW (0)

This is inverted from the expected behavior.

### The Software Fix
- `cosmolab.json`: Uses `CD4021Switch` component (normal polarity)
- `cosmolab_hw_v1.json`: Uses `CD4021SwitchInverted` component (compensates for inverted polarity)

### Why Two Files?
Having separate files allows:
1. **Plugdata compatibility** - simulation works correctly with normal polarity
2. **Hardware v1 support** - actual hardware works with inverted polarity
3. **Future-proof** - clean migration path to v2+ hardware
4. **No patch modifications** - your Max patches don't need any changes!

---

## 📚 Related Documentation
- `COSMOLAB_BUTTON_POLARITY_FIX.md` - Technical explanation (English)
- `COSMOLAB_FIX_PULSANTI_IT.md` - Quick guide (Italian)
- `CHANGELOG_CD4021SwitchInverted.md` - Change log

---

## ❓ FAQ

**Q: Which file should I use for testing in Plugdata?**  
A: Always use `cosmolab` (not `cosmolab_hw_v1`)

**Q: My buttons don't work on hardware!**  
A: Make sure you're compiling with `--board cosmolab_hw_v1` for hardware v1

**Q: Will I need to change my Max patches?**  
A: No! The board definition handles everything. Your patches work with both.

**Q: What happens when hardware v2 arrives?**  
A: Just switch to compiling with `--board cosmolab` instead of `--board cosmolab_hw_v1`

#include <linux/compiler.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/kvm_host.h>

#include <asm/kvm_emulate.h>
#include <asm/kvm_hyp.h>
#include <asm/kvm_mmu.h>

extern struct vgic_global kvm_vgic_global_state;

static __hyp_text void save_lrs_hyp(struct kvm_vcpu *vcpu, void __iomem *base)
{
	struct vgic_v2_cpu_if *cpu_if = &vcpu->arch.vgic_cpu.vgic_v2;
	u64 used_lrs = vcpu->arch.vgic_cpu.used_lrs;
	u64 elrsr;
	int i;

	elrsr = readl_relaxed(base + GICH_ELRSR0);
	if (unlikely(used_lrs > 32))
		elrsr |= ((u64)readl_relaxed(base + GICH_ELRSR1)) << 32;

	for (i = 0; i < used_lrs; i++) {
		if (elrsr & (1UL << i))
			cpu_if->vgic_lr[i] &= ~GICH_LR_STATE;
		else
			cpu_if->vgic_lr[i] = readl_relaxed(base + GICH_LR0 + (i * 4));

		writel_relaxed(0, base + GICH_LR0 + (i * 4));
	}
}

void __hyp_text __vgic_v2_save_state(struct kvm_vcpu *vcpu){
    // because this function is called in hyp, so we use hyp VA
    // 如果kvm_vgic_global_state不能直接调的话，就通过用参数的方式传递
    struct vgic_global* global_state_hyp = kern_hyp_va(&kvm_vgic_global_state);
    void __iomem *base = (*global_state_hyp).vctrl_hyp;
	u64 used_lrs = vcpu->arch.vgic_cpu.used_lrs;

	if (!base)
		return;

	if (used_lrs) {
		save_lrs_hyp(vcpu, base);
		writel_relaxed(0, base + GICH_HCR);
	}
}


void __hyp_text __vgic_v2_restore_state(struct kvm_vcpu *vcpu){
    struct vgic_v2_cpu_if *cpu_if = &vcpu->arch.vgic_cpu.vgic_v2;
	struct vgic_global* global_state_hyp = kern_hyp_va(&kvm_vgic_global_state);
    void __iomem *base = (*global_state_hyp).vctrl_hyp;
	u64 used_lrs = vcpu->arch.vgic_cpu.used_lrs;
	int i;

	if (!base)
		return;

	if (used_lrs) {
		writel_relaxed(cpu_if->vgic_hcr, base + GICH_HCR);
		for (i = 0; i < used_lrs; i++) {
			writel_relaxed(cpu_if->vgic_lr[i],
				       base + GICH_LR0 + (i * 4));
		}
	}
}
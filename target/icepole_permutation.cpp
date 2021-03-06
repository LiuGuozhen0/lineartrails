#include "icepole_permutation.h"


IcepolePermutation::IcepolePermutation(unsigned int rounds) : Permutation(rounds) {
  for(unsigned int i = 0; i< 2*rounds +1; ++i){
      this->state_masks_[i].reset(new IcepoleState);
      this->saved_state_masks_[i].reset(new IcepoleState);
    }
  for (unsigned int i = 0; i < rounds; ++i) {
    this->linear_layers_[i].reset(new IcepoleLinearLayer);
    this->linear_layers_[i]->SetMasks(this->state_masks_[2*i].get(), this->state_masks_[2*i + 1].get());
    this->sbox_layers_[i].reset(new IcepoleSboxLayer);
    this->sbox_layers_[i]->SetMasks(this->state_masks_[2*i + 1].get(), this->state_masks_[2*i + 2].get());
    this->saved_linear_layers_[i].reset(new IcepoleLinearLayer);
    this->saved_linear_layers_[i]->SetMasks(this->saved_state_masks_[2*i].get(), this->saved_state_masks_[2*i + 1].get());
    this->saved_sbox_layers_[i].reset(new IcepoleSboxLayer);
    this->saved_sbox_layers_[i]->SetMasks(this->saved_state_masks_[2*i + 1].get(), this->saved_state_masks_[2*i + 2].get());
  }
  touchall();
}


IcepolePermutation::IcepolePermutation(const IcepolePermutation& other) : Permutation(other.rounds_) {
  for (unsigned int i = 0; i < 2 * rounds_ + 1; ++i) {
    this->state_masks_[i].reset(other.state_masks_[i]->clone());
  }

  for (unsigned int i = 0; i < rounds_; ++i) {
    this->sbox_layers_[i].reset(other.sbox_layers_[i]->clone());
    this->sbox_layers_[i]->SetMasks(this->state_masks_[2 * i +1].get(),
                                    this->state_masks_[2 * i + 2].get());
    this->linear_layers_[i].reset(other.linear_layers_[i]->clone());
    this->linear_layers_[i]->SetMasks(this->state_masks_[2 * i + 0].get(),
                                      this->state_masks_[2 * i + 1].get());
  }
  this->toupdate_linear = other.toupdate_linear;
  this->toupdate_nonlinear = other.toupdate_nonlinear;

  for(unsigned int i = 0; i< 2*rounds_ +1; ++i){
      this->saved_state_masks_[i].reset(new IcepoleState);
    }
  for (unsigned int i = 0; i < rounds_; ++i) {
    this->saved_linear_layers_[i].reset(new IcepoleLinearLayer);
    this->saved_linear_layers_[i]->SetMasks(this->saved_state_masks_[2*i].get(), this->saved_state_masks_[2*i + 1].get());
    this->saved_sbox_layers_[i].reset(new IcepoleSboxLayer);
    this->saved_sbox_layers_[i]->SetMasks(this->saved_state_masks_[2*i + 1].get(), this->saved_state_masks_[2*i + 2].get());
  }
}


void IcepolePermutation::set(Permutation* perm) {
  for (unsigned int i = 0; i < 2 * rounds_ + 1; ++i) {
    this->state_masks_[i].reset(perm->state_masks_[i]->clone());
  }

  for (unsigned int i = 0; i < rounds_; ++i) {
    this->sbox_layers_[i].reset(perm->sbox_layers_[i]->clone());
    this->sbox_layers_[i]->SetMasks(this->state_masks_[2 * i +1].get(),
                                    this->state_masks_[2 * i + 2].get());
    this->linear_layers_[i].reset(perm->linear_layers_[i]->clone());
    this->linear_layers_[i]->SetMasks(this->state_masks_[2 * i + 0].get(),
                                      this->state_masks_[2 * i + 1].get());
  }
  this->toupdate_linear = perm->toupdate_linear;
  this->toupdate_nonlinear = perm->toupdate_nonlinear;
}


IcepolePermutation& IcepolePermutation::operator=(const IcepolePermutation& rhs){
  for(unsigned int i = 0; i< 2*rounds_ +1; ++i){
    this->state_masks_[i].reset(rhs.state_masks_[i]->clone());
  }
  this->toupdate_linear = rhs.toupdate_linear;
 this->toupdate_nonlinear = rhs.toupdate_nonlinear;
 for (unsigned int i = 0; i < rounds_; ++i) {
   this->linear_layers_[i].reset(new IcepoleLinearLayer);
   this->linear_layers_[i]->SetMasks(this->state_masks_[2*i].get(), this->state_masks_[2*i + 1].get());
   this->sbox_layers_[i].reset(new IcepoleSboxLayer);
   this->sbox_layers_[i]->SetMasks(this->state_masks_[2*i + 1].get(), this->state_masks_[2*i + 2].get());
 }
 return *this;
}


bool IcepolePermutation::update() {
  std::unique_ptr<StateMaskBase> tempin, tempout;
  bool update_before, update_after;
  while (this->toupdate_linear == true || this->toupdate_nonlinear == true) {
    if (this->toupdate_nonlinear == true) {
      this->toupdate_nonlinear = false;
      for (unsigned int layer = 0; layer < rounds_; ++layer) {
        tempin.reset(this->sbox_layers_[layer]->in->clone());
        tempout.reset(this->sbox_layers_[layer]->out->clone());
        if (this->sbox_layers_[layer]->Update() == false)
          return false;
        update_before = this->sbox_layers_[layer]->in->diffLinear(*(tempin));
        update_after = this->sbox_layers_[layer]->out->diffLinear(*(tempout));
//        update_before = this->sbox_layers_[layer]->in->changesforLinear();
//        update_after = this->sbox_layers_[layer]->out->changesforLinear();
        if(((update_after == true) && (layer != rounds_ - 1)) ||  update_before)
          this->toupdate_linear = true;
      }
    }
    if (this->toupdate_linear == true) {
      this->toupdate_linear = false;
      for (unsigned int layer = 0; layer < rounds_; ++layer) {
        tempin.reset(this->linear_layers_[layer]->in->clone());
        tempout.reset(this->linear_layers_[layer]->out->clone());
        if (this->linear_layers_[layer]->Update() == false)
          return false;
        update_before = this->linear_layers_[layer]->in->diffSbox(*(tempin));
        update_after = this->linear_layers_[layer]->out->diffSbox(*(tempout));
//        update_before = this->sbox_layers_[layer]->in->changesforSbox();
//        update_after = this->sbox_layers_[layer]->out->changesforSbox();
        if(((update_before == true) && (layer != 0)) ||  update_after)
          this->toupdate_nonlinear = true;
    }
  }
  }
  return true;
}


Permutation::PermPtr IcepolePermutation::clone() const {
  return PermPtr(new IcepolePermutation(*this));
}


void IcepolePermutation::PrintWithProbability(std::ostream& stream,
                                              unsigned int offset) {
  Permutation::PrintWithProbability(stream, 1);
}


void IcepolePermutation::touchall() {
  Permutation::touchall();
}


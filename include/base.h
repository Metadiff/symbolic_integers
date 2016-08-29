//
// Created by alex on 24/08/16.
//

#ifndef METADIFF_SYMBOLIC_INTEGERS_TEMPLATED_BASE_H
#define METADIFF_SYMBOLIC_INTEGERS_TEMPLATED_BASE_H
namespace md {
    namespace sym {
        C floor(C dividend, C divisor) {
            if(divisor == 0){
                throw DivisionByZero();
            }
            if((dividend >= 0 and divisor > 0)
               or (dividend <= 0 and divisor < 0)
               or (dividend % divisor == 0)){
                return (dividend / divisor);
            }
            return dividend / divisor - 1;
        };

        C ceil(C dividend, C divisor){
            if(divisor == 0){
                throw DivisionByZero();
            }
            if((dividend >= 0 and divisor < 0) or (dividend <= 0 and divisor > 0)){
                return dividend / divisor;
            }
            if(dividend % divisor == 0){
                return dividend / divisor;
            }
            return dividend / divisor + 1;
        };


        std::vector <std::pair<I, std::pair < Polynomial, Polynomial>>>
                Polynomial::floor_registry;
        std::vector <std::pair<I, std::pair < Polynomial, Polynomial>>>
                Polynomial::ceil_registry;
        Polynomial Polynomial::zero = Polynomial(0);
        Polynomial Polynomial::one = Polynomial(1);

        C Monomial::eval(const std::vector <C> &values) const {
            C value = coefficient;
            for (auto i = 0; i < powers.size(); ++i) {
                C cur_value;
                auto floor_poly = Polynomial::get_floor(powers[i].first);
                auto ceil_poly = Polynomial::get_ceil(powers[i].first);
                if (floor_poly.first != 0) {
                    C dividend = floor_poly.second.first.eval(values);
                    C divisor = floor_poly.second.second.eval(values);
                    cur_value = floor(dividend, divisor);
                } else if (ceil_poly.first != 0) {
                    C dividend = ceil_poly.second.first.eval(values);
                    C divisor = ceil_poly.second.second.eval(values);
                    cur_value = ceil(dividend, divisor);
                } else if (values.size() <= powers[i].first) {
                    throw EvaluationFailure();
                } else {
                    cur_value = values[powers[i].first];
                }
                value *= pow(cur_value, powers[i].second);
            }
            return value;
        }

        C Monomial::eval(const std::vector <std::pair<I, C>> &values) const {
            C value = coefficient;
            for (auto i = 0; i < powers.size(); ++i) {
                C cur_value;
                auto floor_poly = Polynomial::get_floor(powers[i].first);
                auto ceil_poly = Polynomial::get_ceil(powers[i].first);
                if (floor_poly.first != 0) {
                    C dividend = floor_poly.second.first.eval(values);
                    C divisor = floor_poly.second.second.eval(values);
                    cur_value = floor(dividend, divisor);
                } else if (ceil_poly.first != 0) {
                    C dividend = ceil_poly.second.first.eval(values);
                    C divisor = ceil_poly.second.second.eval(values);
                    cur_value = ceil(dividend, divisor);
                } else {
                    bool found = false;
                    for(auto j = 0; j < values.size(); ++j){
                        if(values[j].first == powers[i].first){
                            cur_value = values[j].second;
                            found = true;
                            break;
                        }
                    }
                    if(not found){
                        throw EvaluationFailure();
                    }
                }
                value *= pow(cur_value, powers[i].second);
            }
            return value;
        }


        void reduce_polynomials(std::vector <std::pair<Polynomial, C>> &polynomials,
                                const std::vector<std::pair<I, C>>& values,
                                I id, C value){
            for(auto i = 0; i < polynomials.size(); ++i){
                for(auto j = 0; j < polynomials[i].first.monomials.size(); ++j){
                    for(auto v = 0; v < polynomials[i].first.monomials[j].powers.size(); ++v){
                        auto var_id = polynomials[i].first.monomials[j].powers[v].first;
                        auto var_p = polynomials[i].first.monomials[j].powers[v].second;
                        if(var_id == id){
                            polynomials[i].first.monomials[j].coefficient *= pow(value, var_p);
                            polynomials[i].first.monomials[j].powers.erase(polynomials[i].first.monomials[j].powers.begin() + v);
                            --v;
                            continue;
                        }
                        if(Polynomial::get_floor(var_id).first != 0){
                            try {
                                auto floor_value = Polynomial::specific_variable(var_id).eval(values);
                                polynomials[i].first.monomials[j].coefficient *= pow(floor_value, var_p);
                                polynomials[i].first.monomials[j].powers.erase(polynomials[i].first.monomials[j].powers.begin() + v);
                                --v;
                                continue;
                            } catch (...) {}
                        }
                        if(Polynomial::get_ceil(var_id).first != 0) {
                            try {
                                auto ceil_value = Polynomial::specific_variable(var_id).eval(values);
                                polynomials[i].first.monomials[j].coefficient *= pow(ceil_value, var_p);
                                polynomials[i].first.monomials[j].powers.erase(polynomials[i].first.monomials[j].powers.begin() + v);
                                --v;
                                continue;
                            } catch (...) {}
                        }
                    }
                    // Check if the monomial is the up to a constant to some previous and combine if so
                    for(auto k = 0; k < j; ++k){
                        if(up_to_coefficient(polynomials[i].first.monomials[j], polynomials[i].first.monomials[k])){
                            polynomials[i].first.monomials[k].coefficient += polynomials[i].first.monomials[j].coefficient;
                            polynomials[i].first.monomials.erase(polynomials[i].first.monomials.begin()+j);
                            --j;
                            break;
                        }
                    }
                }
            }
        };

        std::vector<std::pair<I, C>> Polynomial::deduce_values(
                const std::vector <std::pair<Polynomial, C>> &implicit_values){
            std::vector<std::pair<Polynomial, C>> work = implicit_values;
            std::vector<std::pair<I, C>> values;
            for(auto i = 0; i < work.size(); ++i){
                // Remove constant polynomials
                if(work[i].first.is_constant()){
                    if(work[i].first.eval() != work[i].second){
                        throw EvaluationFailure();
                    }
                    work.erase(work.begin() + i);
                    --i;
                    continue;
                }
                // Eliminate constant monomials from the current one
                for(auto j = 0; j < work[i].first.monomials.size(); ++j){
                    if(work[i].first.monomials[j].is_constant()){
                        work[i].second -= work[i].first.monomials[j].eval();
                        work[i].first.monomials.erase(work[i].first.monomials.begin() + j);
                        --j;
                    }
                }
                if(work[i].first.monomials.size() == 1 and work[i].first.monomials[0].powers.size() == 1){
                    // This means that the polynomial is of the form c * x^n and we can evaluate it
                    C c = work[i].first.monomials[0].coefficient;
                    I id = work[i].first.monomials[0].powers[0].first;
                    P power = work[i].first.monomials[0].powers[0].second;
                    C value = C(pow(work[i].second / c, 1.0 / power));
                    if(pow(value, power) * c != work[i].second){
                        throw EvaluationFailure();
                    }
                    // Add to the values
                    values.push_back({id, value});
                    // Remove current polynomial from list
                    work.erase(work.begin() + i);
                    // Reduce all other polynomials
                    if(work.size() == 0){
                        break;
                    }
                    reduce_polynomials(work, values, id, value);
                    // Start from begin again
                    i = -1;
                }
            }
            if(work.size() > 0){
                throw EvaluationFailure();
            }
            return values;
        }

        std::string Monomial::to_string() const {
            if (powers.size() == 0) {
                return std::to_string(coefficient);
            }

            std::string result;
            if (coefficient != 1) {
                if (coefficient == -1) {
                    result += "-";
                } else {
                    result += std::to_string(coefficient);
                }
            }
            std::pair <I, std::pair<Polynomial, Polynomial>> floor_value {0, {Polynomial(0), Polynomial(0)}};
            std::pair <I, std::pair<Polynomial, Polynomial>> ceil_value {0, {Polynomial(0), Polynomial(0)}};
            for (auto i = 0; i < powers.size(); ++i) {
                if ((floor_value = Polynomial::get_floor(powers[i].first)).first != 0) {
                    result += "floor(" + floor_value.second.first.to_string() + " / " +
                              floor_value.second.second.to_string() + ")";
                } else if ((ceil_value = Polynomial::get_ceil(powers[i].first)).first != 0) {
                    result += "ceil(" + ceil_value.second.first.to_string() + " / " +
                              ceil_value.second.second.to_string() + ")";
                } else {
                    result += ('a' + powers[i].first);
                    auto n = powers[i].second;
                    std::string super_scripts;
                    while (n > 0) {
                        auto reminder = n % 10;
                        n /= 10;
                        switch (reminder) {
                            case 0:
                                super_scripts = "\u2070" + super_scripts;
                                break;
                            case 1:
                                super_scripts = "\u00B9" + super_scripts;
                                break;
                            case 2:
                                super_scripts = "\u00B2" + super_scripts;
                                break;
                            case 3:
                                super_scripts = "\u00B3" + super_scripts;
                                break;
                            case 4:
                                super_scripts = "\u2074" + super_scripts;
                                break;
                            case 5:
                                super_scripts = "\u2075" + super_scripts;
                                break;
                            case 6:
                                super_scripts = "\u2076" + super_scripts;
                                break;
                            case 7:
                                super_scripts = "\u2077" + super_scripts;
                                break;
                            case 8:
                                super_scripts = "\u2078" + super_scripts;
                                break;
                            case 9:
                                super_scripts = "\u2079" + super_scripts;
                                break;
                        }
                    }
                    result += super_scripts;
                }
            }
            return result;
        }

        std::ostream &operator<<(std::ostream &f, const Monomial &monomial) {
            return f << monomial.to_string();
        }

        std::ostream &operator<<(std::ostream &f, const Polynomial &polynomial) {
            return f << polynomial.to_string();
        }
    }
}
#endif //METADIFF_SYMBOLIC_INTEGERS_TEMPLATED_BASE_H

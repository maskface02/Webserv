/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 02:57:27 by zatais            #+#    #+#             */
/*   Updated: 2026/05/11 02:57:29 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.hpp"

int ft_isspace(int c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
          c == '\v');
}

int ft_isdigit(int c) { return (c >= '0' && c <= '9'); }

int ft_toupper(int c) { return (c >= 'a' && c <= 'z') ? (c - 32) : c; }

int ft_isNumberOverflow(long res, int last_digit) {
  long max_threshold = LONG_MAX / 10;
  int max_last_digit = 7;
  if (res > max_threshold || (res == max_threshold && last_digit > max_last_digit))
    return 1;
  return 0;
}

long ft_atol(std::string &s, int &isOverflowed) {
  int limit = 0;
  long value = 0;
  if (isOverflowed == 99)
    limit = 1000;
  for (size_t j = 0; j < s.size(); ++j) {
    int digit = s[j] - '0';
    if ((limit && value > limit) || ft_isNumberOverflow(value, digit)) {
      isOverflowed = 1;
      return -1;
    }
    value = value * 10 + digit;
  }
  isOverflowed = 0;
  return value;
}

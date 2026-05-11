/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/10 21:22:37 by zatais            #+#    #+#             */
/*   Updated: 2026/05/11 02:57:19 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HELPERS_HPP
#define HELPERS_HPP
#include <climits>
#include <cstddef>
#include <string>

int ft_isspace(int c);
int ft_isdigit(int c);
int ft_toupper(int c);
long ft_atol(std::string &s, int &isOverflowed);
int ft_isNumberOverflow(long res, int last_digit);
#endif
